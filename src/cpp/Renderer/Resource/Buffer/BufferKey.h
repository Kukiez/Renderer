#pragma once
#include "MappedBufferRange.h"
#include <algorithm>
#include <atomic>

class BufferKey;
class BufferResourceStorage;

enum class BufferUsageHint : uint8_t {
    PERSISTENT_READ_WRITE,
    PERSISTENT_READ_ONLY,
    PERSISTENT_WRITE_ONLY,
    IMMUTABLE,
    FRAME_SCRATCH_BUFFER
};

struct BufferDescriptor {
    size_t sizeBytes{};
};

struct GPUBuffer {
    unsigned gpuSSBO{};
    BufferDescriptor descriptor;
    BufferResourceStorage* ref{};
    BufferUsageHint usage{};
    char* mapped{};
    std::atomic<int> uses = 1;
};

struct BufferBlockData {
    GPUBuffer* backingBuffer{};
    size_t offset{};
    size_t size{};

    void clamp(size_t& targetOffset, size_t& targetSize) const {
        targetOffset = std::min(targetOffset, size);
        targetSize = std::min(targetSize, size - targetOffset);
    }

    AMappedBufferRange createStagingBufferForRange(size_t byteOffset, size_t byteSize);

    void destroyBuffer(BufferKey& buffer) const;

    void resize(BufferKey& buffer, size_t copySrcOffset, size_t copyDstOffset, size_t copyBytes, size_t newBufferCapacity) const;
};

class BufferKey {
    friend class BufferResourceStorage;
    friend class GraphicsContext;

    BufferBlockData* buffer{};
public:
    constexpr BufferKey() = default;

    constexpr BufferKey(BufferBlockData* buffer) : buffer(buffer) {}

    size_t size() const { return buffer->size; }

    template <typename T>
    size_t sizeIn() const { return size() / sizeof(T); }

    size_t sizeIn(size_t divisor) {
        return size() / divisor;
    }

    template <typename T>
    TMappedBufferRange<T> mapRange(const size_t tOffset = 0, const size_t tSize = 0) {
        return buffer->createStagingBufferForRange(tOffset * sizeof(T), tSize * sizeof(T)).as<T>();
    }

    bool operator == (const BufferKey& other) const { return buffer == other.buffer; }
    bool operator != (const BufferKey& other) const { return !(*this == other); }

    size_t getOffset() const { return buffer->offset; }

    BufferUsageHint usage() const { return buffer->backingBuffer->usage; }

    BufferBlockData* _getInternalBufferBlock() const { return buffer; }

    operator bool() const { return buffer != nullptr; }

    bool isValid() const { return buffer != nullptr; }

    void destroy() {
        if (buffer) buffer->destroyBuffer(*this);
    }

    BufferKey resize(const size_t copySrcOffset, const size_t copyDstOffset, size_t copyBytes, size_t newBufferCapacity) {
        if (buffer) buffer->resize(*this, copySrcOffset, copyDstOffset, copyBytes, newBufferCapacity);
        return {};
    }
};

static constexpr auto NULL_BUFFER_KEY = BufferKey{};