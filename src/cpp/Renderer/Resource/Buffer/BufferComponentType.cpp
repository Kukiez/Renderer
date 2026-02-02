#include "BufferComponentType.h"
#include <gl/glew.h>

static auto DEBUG_EXTRA_BYTES = 0;

GLbitfield ToGLBufferStorageFlags(BufferUsageHint usage) {
    switch (usage) {
        case BufferUsageHint::PERSISTENT_READ_WRITE:
            return GL_MAP_PERSISTENT_BIT |
                   GL_MAP_COHERENT_BIT |
                   GL_MAP_READ_BIT |
                   GL_MAP_WRITE_BIT |
                   GL_DYNAMIC_STORAGE_BIT;

        case BufferUsageHint::PERSISTENT_READ_ONLY:
            return GL_MAP_PERSISTENT_BIT |
                   GL_MAP_COHERENT_BIT |
                   GL_MAP_READ_BIT |
                   GL_DYNAMIC_STORAGE_BIT;

        case BufferUsageHint::PERSISTENT_WRITE_ONLY:
        case BufferUsageHint::FRAME_SCRATCH_BUFFER:
            return GL_MAP_PERSISTENT_BIT |
                   GL_MAP_WRITE_BIT |
                   GL_MAP_COHERENT_BIT |
                   GL_DYNAMIC_STORAGE_BIT;

        case BufferUsageHint::IMMUTABLE:
            return GL_DYNAMIC_STORAGE_BIT;
    }
    assert(false);
    std::unreachable();
}

GLbitfield ToGLMapFlags(BufferUsageHint usage) {
    switch (usage) {
        case BufferUsageHint::PERSISTENT_READ_WRITE:
            return GL_MAP_READ_BIT |
                   GL_MAP_WRITE_BIT |
                   GL_MAP_PERSISTENT_BIT |
                   GL_MAP_COHERENT_BIT;
        case BufferUsageHint::PERSISTENT_WRITE_ONLY:
        case BufferUsageHint::FRAME_SCRATCH_BUFFER:
            return GL_MAP_WRITE_BIT |
                   GL_MAP_PERSISTENT_BIT |
                   GL_MAP_COHERENT_BIT;
    }
    assert(false);
    std::unreachable();
}

AMappedBufferRange BufferBlockData::createStagingBufferForRange(size_t byteOffset, size_t byteSize) {
    clamp(byteOffset, byteSize);

    if (backingBuffer->usage == BufferUsageHint::IMMUTABLE) {
        return backingBuffer->ref->createStagingRegion(this, byteOffset, byteSize);
    }
    char* mappedBf = backingBuffer->mapped;

    if (!mappedBf) { // not yet created
        return backingBuffer->ref->createStagingRegion(this, byteOffset, byteSize);
    }
    auto range = AMappedBufferRange(mappedBf + offset + byteOffset, byteSize);

    backingBuffer->ref->createStagingRegionOfExistingMemory(this, byteOffset, range);
    return range;
}

void BufferBlockData::destroyBuffer(BufferKey &buffer) const {
    backingBuffer->ref->destroyBuffer(buffer);
    buffer = {};
}

void BufferBlockData::resize(BufferKey &buffer, size_t copySrcOffset, size_t copyDstOffset, size_t copyBytes,
    size_t newBufferCapacity) const {
    auto old = buffer;
    buffer = backingBuffer->ref->copyFromBuffer(buffer, newBufferCapacity, copySrcOffset, copyDstOffset, copyBytes);
    backingBuffer->ref->destroyBuffer(old);
}

BufferResourceStorage::BufferResourceStorage() = default;

void CreateSSBO(GPUBuffer* buffer, AMappedBufferRange initialData) {
    glGenBuffers(1, &buffer->gpuSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer->gpuSSBO);

    const auto sizeBytes = static_cast<long long>(buffer->descriptor.sizeBytes);

    glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeBytes, initialData.data(), ToGLBufferStorageFlags(buffer->usage));

    if (buffer->usage != BufferUsageHint::IMMUTABLE) {
        buffer->mapped = static_cast<char *>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeBytes, ToGLMapFlags(buffer->usage)));

        assert(buffer->mapped);
    }
}

void BufferResourceStorage::initialize() {
    for (auto& frameBuffer : frameBuffers) {
        frameBuffer.descriptor.sizeBytes = 4 * 1024 * 1024;
        frameBuffer.usage = BufferUsageHint::FRAME_SCRATCH_BUFFER;
        frameBuffer.ref = this;

        CreateSSBO(&frameBuffer, {});
    }
}

void BufferResourceStorage::synchronizeGpuBuffers() {
    for (auto& [initialData, newGpuBuffer] : this->newGpuBlocks) {
        CreateSSBO(newGpuBuffer, initialData);
    }

    newGpuBlocks.clear();

    for (auto& staging : stagingRegions) {
        auto buffer = staging.buffer;

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer->gpuSSBO);

        if (buffer->usage == BufferUsageHint::IMMUTABLE) {
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, staging.offset, staging.data.size(), staging.data.data());
        } else {
            char* mapped = buffer->mapped + staging.offset;
            std::memcpy(mapped, staging.data.data(), staging.data.size());

            // TODO use explicit flushing
        //    glFlushMappedBufferRange(GL_SHADER_STORAGE_BUFFER, staging.offset, staging.data.size());
        }
    }

    stagingRegions.clear();

    for (auto& copyRegion : copyRegions) {
        auto srcBuffer = copyRegion.src.buffer->backingBuffer->gpuSSBO;
        auto dstBuffer = copyRegion.dst.buffer->backingBuffer->gpuSSBO;

        glBindBuffer(GL_COPY_READ_BUFFER, srcBuffer);
        glBindBuffer(GL_COPY_WRITE_BUFFER, dstBuffer);

        long long srcOffset = static_cast<long long>(copyRegion.srcOffset);
        long long dstOffset = static_cast<long long>(copyRegion.dstOffset);
        long long sizeInBytes = static_cast<long long>(copyRegion.sizeInBytes);

        glCopyBufferSubData(
            GL_COPY_READ_BUFFER,
            GL_COPY_WRITE_BUFFER,
            srcOffset,
            dstOffset,
            sizeInBytes
        );
    }

    copyRegions.clear();

    auto& gpuBufferDestructionQ = destructionQueue[clearDestructionIdx];
    gpuBufferDestructionQ.reserve(toDestroy.size());

    for (const auto& buffer : toDestroy) {
        int uses = buffer->backingBuffer->uses.fetch_sub(1);

        if (uses == 1) {
            gpuBufferDestructionQ.emplace_back(buffer->backingBuffer);
        }
        freeBuffers.push(buffer);
    }
    toDestroy = {};

    for (auto& stagingAllocator : stagingAllocators) {
        stagingAllocator.reset();
    }
}

void BufferResourceStorage::onFrameFinished() {
    for (auto& [st, ptr] : debugFrameBuffer) {
        for (int i = 0; i < DEBUG_EXTRA_BYTES; ++i) {
            if (ptr[i] != 55) {
                std::cout << "Stacktrace: [ " << i << "]: " << st << std::endl;
                assert(false);
            }
        }
    }
    debugFrameBuffer.clear();

    for (auto& queue : destructionQueue[clearDestructionIdx]) {
        if (queue->mapped) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, queue->gpuSSBO);
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
        glDeleteBuffers(1, &queue->gpuSSBO);
        std::memset(queue, 0, sizeof(GPUBuffer));
        freeGpuBuffers.push(queue);
    }

    frameBufferNext[currentFrame] = 0;

    currentFrame = (currentFrame + 1) % 3;

    destructionQueue[clearDestructionIdx].clear();
    clearDestructionIdx = (clearDestructionIdx + 1) % destructionQueue.size();
    pushDestructionIdx = (pushDestructionIdx + 1) % destructionQueue.size();

    frameBufferAllocator.clear();
}

GPUBuffer * BufferResourceStorage::createGPUBuffer(size_t bytes, BufferUsageHint usage) {
    GPUBuffer* buffer = nullptr;

    if (freeGpuBuffers.tryPop(buffer)) {
    } else {
        buffer = &*gpuBufferAllocator.emplace_back();
        buffer->usage = usage;
        buffer->ref = this;
        buffer->descriptor.sizeBytes = bytes;
        buffer->mapped = nullptr;
    }
    return buffer;
}

std::pair<BufferBlockData *, AMappedBufferRange> BufferResourceStorage::createFrameBlock(size_t bytes) {
    const uint32_t totalBytes = bytes + DEBUG_EXTRA_BYTES;

    size_t offset = frameBufferNext[currentFrame].fetch_add(totalBytes, std::memory_order_relaxed);

    assert(offset + totalBytes <= frameBuffers[currentFrame].descriptor.sizeBytes);

    char* ptr = frameBuffers[currentFrame].mapped + offset;

    std::memset(ptr + bytes, 55, DEBUG_EXTRA_BYTES);

    GPUBuffer* buffer = &frameBuffers[currentFrame];

    debugFrameBuffer.emplace_back(std::stacktrace::current(), ptr + bytes);

    BufferBlockData block;
    block.size = bytes;
    block.offset = offset;
    block.backingBuffer = buffer;
    return {&*frameBufferAllocator.emplace_back(block), AMappedBufferRange(ptr, bytes)};
}

BufferBlockData * BufferResourceStorage::createBlock(size_t bytes, BufferUsageHint usage, AMappedBufferRange initialData) {
    bytes = alignTo16(bytes);

    size_t offset = 0;

    GPUBuffer* buffer = nullptr;
    BufferBlockData* availBlock{};

    if (freeBuffers.tryPop(availBlock)) {
    } else {
        availBlock = &*bufferBlocksAllocator.emplace_back();
    }
    buffer = createGPUBuffer(bytes, usage);
    new (availBlock) BufferBlockData(buffer, offset, bytes);

    newGpuBlocks.emplace_back(initialData, buffer);
    return availBlock;
}

BufferKey BufferResourceStorage::createBuffer(size_t bytes, const BufferUsageHint usage) {
    auto block = createBlock(bytes, usage);
    return {block};
}

void BufferResourceStorage::destroyBuffer(BufferKey buffer) {
    if (!buffer) {
        return;
    }
    if (buffer.usage() == BufferUsageHint::FRAME_SCRATCH_BUFFER) {
        assert(false && "Frame scratch buffers cant be destroyed");
        return;
    }
    toDestroy.emplace_back(buffer._getInternalBufferBlock());
}

std::pair<BufferKey, AMappedBufferRange> BufferResourceStorage::createBufferWithData(size_t bytes,
                                                                                     BufferUsageHint usage)
{
    bytes = alignTo16(bytes);

    if (usage == BufferUsageHint::FRAME_SCRATCH_BUFFER) {
        return createFrameBlock(bytes);
    }
    auto* mem = stagingAllocators.local().allocate(bytes);
    AMappedBufferRange range(mem, bytes);
    auto block = createBlock(bytes, usage, range);
    return {BufferKey{block}, range};
}

AMappedBufferRange BufferResourceStorage::createStagingRegion(BufferBlockData *block, size_t offset, size_t bytes) {
    const size_t bufferSize = block->size;

    if (bytes == -1) {
        bytes = bufferSize;
        bytes -= offset;
    } else {
        offset = std::min(offset, bufferSize);
        bytes = std::min(bytes, bufferSize - offset);
    }
    bytes = alignTo16(bytes);

    auto* mem = stagingAllocators.local().allocate(bytes);
    AMappedBufferRange range(mem, bytes);

    stagingRegions.emplace_back(StagingBuffer{block->backingBuffer, block->offset + offset, range});
    return range;
}

void BufferResourceStorage::createStagingRegionOfExistingMemory(const BufferBlockData *block, size_t offset, AMappedBufferRange range) {
    stagingRegions.emplace_back(StagingBuffer{block->backingBuffer, block->offset + offset, range});
}

void BufferResourceStorage::copyBufferRange(CopyBufferRange copy) {
    size_t _ = 0;

    copy.src.buffer->clamp(_, copy.sizeInBytes);
    copy.dst.buffer->clamp(copy.dstOffset, copy.sizeInBytes);

    if (copy.src.buffer == copy.dst.buffer) {
    }

    if (copy.sizeInBytes == 0) {
        return;
    }
    copyRegions.emplace_back(copy);
}

BufferKey BufferResourceStorage::copyFromBuffer(BufferKey buffer, size_t newSize, size_t copySrcOffset, size_t copyDstOffset,
    size_t copySize) {
    if (buffer.usage() == BufferUsageHint::FRAME_SCRATCH_BUFFER) {
        assert(false && "Frame scratch buffers cant be resized");
        return {};
    }
    auto newBuffer = createBuffer(newSize, buffer.usage());
    copyBufferRange({
        .src = buffer,
        .dst = newBuffer,
        .srcOffset = copySrcOffset,
        .dstOffset = copyDstOffset,
        .sizeInBytes = copySize
    });
    return newBuffer;
}