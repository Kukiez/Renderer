#pragma once
#include <memory/free_list_allocator.h>
#include <memory/byte_arena.h>
#include <tbb/tbb.h>
#include <unordered_map>
#include <memory/vector.h>

struct ThreadLocalChunk {
    struct Free {
        void* ptr = 0;
        size_t count = 0;
    };
    mem::free_list_allocator allocator;
    tbb::concurrent_vector<Free> frees[2];
    std::atomic<uint8_t> writeIndex = 0;

    ThreadLocalChunk() : allocator(1 * 1024 * 1024) {
        frees[0].reserve(16);
        frees[1].reserve(16);
    }

    ThreadLocalChunk(ThreadLocalChunk&& other) noexcept
    : allocator(std::move(other.allocator)), writeIndex(other.writeIndex.load()) {
        frees[0] = std::move(other.frees[0]);
        frees[1] = std::move(other.frees[1]);
        other.writeIndex = 0;
    }


    ThreadLocalChunk& operator=(ThreadLocalChunk&& other) noexcept {
        if (this != &other) {
            allocator = std::move(other.allocator);
            writeIndex = other.writeIndex.load();
            frees[0] = std::move(other.frees[0]);
            frees[1] = std::move(other.frees[1]);
            other.writeIndex = 0;
        }
        return *this;
    }

    void merge() {
        auto wi = writeIndex.load();
        for (auto& free : frees[!wi]) {
            allocator.deallocate(free.ptr, free.count);
        }
        frees[!wi].clear();

        writeIndex.store(!wi);
    }

    void deallocate(void* ptr, size_t count) {
        frees[writeIndex].emplace_back(ptr, count);
    }
};

class ThreadAllocator {
    tbb::enumerable_thread_specific<ThreadLocalChunk> threads;
public:
    ThreadAllocator() = default;
    
    void* allocate(const mem::type_info* type, size_t count) {
        return threads.local().allocator.allocate(type, count);
    }

    void deallocate(void* ptr, const size_t len) {
        if (!threads.local().allocator.deallocate(ptr, len)) {
            for (auto& thread : threads) {
                if (thread.allocator.does_pointer_belong_here(ptr)) {
                    thread.deallocate(ptr, len);
                }
            }
        }
    }

    void deallocatePending() {
        for (auto& thread : threads) {
            thread.merge();
        }
    }
};