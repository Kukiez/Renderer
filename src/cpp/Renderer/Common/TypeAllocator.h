#pragma once
#include <memory/type_info.h>
#include <vector>

struct TypeAllocator {
    char* ptr{};
    size_t size{};
    size_t capacity{};

    bool isFull() const {
        return size == capacity;
    }
};

struct TypeAllocatorArray {
    mem::typeindex type;
    std::vector<TypeAllocator> allocators{};
    std::vector<void*> freeIndices{};

    int size() const {
        return static_cast<int>(allocators.size());
    }

    TypeAllocator& operator[](size_t index) {
        return allocators[index];
    }

    void* find() {
        if (!freeIndices.empty()) {
            void* last = freeIndices.back();
            freeIndices.pop_back();
            return last;
        }
        if (allocators.empty()) {
            return nullptr;
        }

        auto& allocator = allocators.back();

        if (allocator.isFull()) {
            return nullptr;
        }
        return type.index(allocator.ptr, allocator.size++);
    }

    void free(void* ptr) {
        type.destroy(ptr, 1);
        freeIndices.push_back(ptr);
    }
};