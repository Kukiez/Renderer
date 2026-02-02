#pragma once
#include <cstring>

#include "../ResourceKey.h"
#include "BufferKey.h"
#include <memory/AtomicPopList.h>
#include <memory/byte_arena.h>
#include <oneapi/tbb/enumerable_thread_specific.h>

class BufferResourceStorage;
struct LevelContext;
class Level;

struct StagingBuffer {
    GPUBuffer* buffer{};
    size_t offset{};
    AMappedBufferRange data{};
};

struct CopyBufferRange {
    BufferKey src{};
    BufferKey dst{};
    size_t srcOffset{};
    size_t dstOffset{};
    size_t sizeInBytes{};
};

enum class BufferAlignment {
    ALIGN_4,
    ALIGN_8,
    ALIGN_16,
    ALIGN_32,
    GPU_DEFAULT = ALIGN_16
};

class BufferResourceStorage {
    struct ThreadLocalStagingAllocator {
        using Arena = mem::byte_arena<mem::same_alloc_schema, 32>;
        Arena stagingArena = Arena(0.064 * 1024 * 1024);

        char* allocate(size_t bytes) {
            return (char*)stagingArena.allocate(bytes, 8);
        }

        void reset() {
            stagingArena.reset();
        }
    };

    tbb::enumerable_thread_specific<ThreadLocalStagingAllocator> stagingAllocators{};

    GPUBuffer* createGPUBuffer(size_t bytes, BufferUsageHint usage);

    std::pair<BufferBlockData*, AMappedBufferRange> createFrameBlock(size_t bytes);
    BufferBlockData* createBlock(size_t bytes, BufferUsageHint usage, AMappedBufferRange initialData = {});

    std::array<std::vector<GPUBuffer*>, 3> destructionQueue;
    int pushDestructionIdx = 0;
    int clearDestructionIdx = 1;

    tbb::concurrent_vector<std::pair<AMappedBufferRange, GPUBuffer*>> newGpuBlocks{};
    tbb::concurrent_vector<StagingBuffer> stagingRegions;
    tbb::concurrent_vector<CopyBufferRange> copyRegions;

    tbb::concurrent_vector<BufferBlockData> frameBufferAllocator{};
    tbb::concurrent_vector<GPUBuffer> gpuBufferAllocator{};
    tbb::concurrent_vector<BufferBlockData> bufferBlocksAllocator{};

    AtomicPopList<BufferBlockData*> freeBuffers{};
    AtomicPopList<GPUBuffer*> freeGpuBuffers{};

    tbb::concurrent_vector<BufferBlockData*> toDestroy{};

    GPUBuffer frameBuffers[3]{};
    std::atomic<size_t> frameBufferNext[3]{};

    tbb::concurrent_vector<std::pair<std::stacktrace, char*>> debugFrameBuffer{};

    int currentFrame = 0;
public:
    BufferResourceStorage();

    BufferResourceStorage(const BufferResourceStorage&) = delete;
    BufferResourceStorage& operator=(const BufferResourceStorage&) = delete;
    BufferResourceStorage(BufferResourceStorage&&) = delete;
    BufferResourceStorage& operator=(BufferResourceStorage&&) = delete;

    void initialize();
    void synchronizeGpuBuffers();
    void onFrameFinished();

    static size_t alignTo16(size_t bytes) {
        return (bytes + 15) & ~15;
    }

    BufferKey createBuffer(size_t bytes, BufferUsageHint usage);

    void destroyBuffer(BufferKey buffer);

    std::pair<BufferKey, AMappedBufferRange> createBufferWithData(size_t bytes, BufferUsageHint usage);

    template <typename T>
    std::pair<BufferKey, TMappedBufferRange<T>> createBufferWithData(size_t tElements, BufferUsageHint usage) {
        auto [buf, rng] = createBufferWithData(tElements * sizeof(T), usage);
        return {buf, rng.as<T>()};
    }

    AMappedBufferRange createStagingRegion(BufferBlockData* block, size_t offset = 0, size_t bytes = -1);

    AMappedBufferRange createStagingRegion(const BufferKey forBuffer, const size_t offset = 0, const size_t bytes = -1) {
        return createStagingRegion(forBuffer.buffer, offset, bytes);
    }

    template <typename T>
    TMappedBufferRange<T> createStagingRegion(BufferKey buffer, const size_t tOffset = 0, const size_t tSize = -1) {
        return createStagingRegion(buffer, tOffset * sizeof(T), tSize * sizeof(T)).as<T>();
    }

    void createStagingRegionOfExistingMemory(const BufferBlockData* block, size_t offset, AMappedBufferRange range);

    template <typename T>
    BufferKey createBuffer(const size_t tElements, const BufferUsageHint usage) {
        return createBuffer(tElements * sizeof(T), usage);
    }

    void copyBufferRange(CopyBufferRange copy);

    BufferKey copyFromBuffer(BufferKey buffer, size_t newSize, size_t copySrcOffset, size_t copyDstOffset, size_t copySize);
};