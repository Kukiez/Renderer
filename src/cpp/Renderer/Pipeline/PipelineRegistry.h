#pragma once
#include <memory/free_list_allocator.h>

class PipelineRegistry {
    mem::free_list_allocator allocator = mem::free_list_allocator(0.016 * 1024 * 1024);
public:
    PipelineRegistry() = default;

    template <typename T, typename... Args>
    const T* createPipeline(Args&&... args) {
        return new (allocator.allocate<T>(1)) T(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    T* createPassPipeline(Args&&... args) {
        return new (allocator.allocate<T>(1)) T(std::forward<Args>(args)...);
    }
};
