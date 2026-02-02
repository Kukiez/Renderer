#pragma once
#include <memory/free_list_allocator.h>
#include <memory/byte_arena.h>
#include <tbb/tbb.h>
#include <memory/vector.h>
#include <memory/memory.h>

struct ThreadFrameLocalChunk {
    struct AllocationHeader {
        const mem::type_info* type;
        void* begin;
        size_t len;
    };
    mem::byte_arena<> arena = mem::create_byte_arena(mem::megabyte(0.1).bytes());
    mem::vector<AllocationHeader> allocations;

    void* allocate(const mem::type_info* type, size_t count) {
        void* allocation = arena.allocate(type, count);
        if (type->is_trivially_destructible()) return allocation;
        return allocations.emplace_back(type, allocation, count).begin;
    }

    void* allocateUnmanaged(const mem::type_info* type, size_t count) {
        return arena.allocate(type, count);
    }

    ThreadFrameLocalChunk() {}

    ThreadFrameLocalChunk(ThreadFrameLocalChunk&& other) noexcept :
    arena(std::move(other.arena)), allocations(std::move(other.allocations)) {}

    ThreadFrameLocalChunk& operator = (ThreadFrameLocalChunk&& other) noexcept {
        if (this != &other) {
            arena = std::move(other.arena);
            allocations = std::move(other.allocations);
        }
        return *this;
    }

    ~ThreadFrameLocalChunk() {
        destroy();
    }

    void reset() {
        for (auto& [type, ptr, len] : allocations) {
            mem::destroy_at(type, ptr, len);
        }
        arena.reset_compact();
        allocations.clear();
    }

    void destroy() {
        for (auto& [type, ptr, len] : allocations) {
            mem::destroy_at(type, ptr, len);
        }
        arena.destroy();
        allocations.zero_out();
    }
};

enum class FrameAllocationType {
    RAII, /* FrameAllocator calls Destructor of the allocated object */
    UNMANAGED /* Destructor manually managed */
};

class FrameAllocator {
    tbb::enumerable_thread_specific<ThreadFrameLocalChunk> threads;

public:
    FrameAllocator() = default;

    void* allocate(const mem::type_info* type, size_t count) {
        return threads.local().allocate(type, count);
    }

    template <typename T, typename... Args>
    T* allocate(FrameAllocationType allocation, Args&&... args) {
        void* ptr;
        if (allocation == FrameAllocationType::RAII) {
            ptr = allocate(mem::type_info_of<T>, 1);
        } else {
            ptr = allocateUnmanaged(mem::type_info_of<T>, 1);
        }
        return static_cast<T*>(new (ptr) T(std::forward<Args>(args)...));
    }

    void* allocateUnmanaged(const mem::type_info* type, const size_t count) {
        return threads.local().allocateUnmanaged(type, count);
    }

    void reset() {
        for (auto& thread : threads) {
            thread.reset();
        }
    }
};

template <typename T>
struct FrameAllocatorAdaptor : mem::default_construct<T>, mem::default_destroy<T> {
    using value_type = T;

    template <typename U>
    struct rebind {
        using other = FrameAllocatorAdaptor<U>;
    };

    FrameAllocator* allocator = nullptr;

    FrameAllocatorAdaptor() = default;
    FrameAllocatorAdaptor(FrameAllocator* allocator) : allocator(allocator) {}

    template <typename U>
    FrameAllocatorAdaptor(FrameAllocatorAdaptor<U> other) : allocator(other.allocator) {}

    void deallocate(T*, const size_t) {}

    T* allocate(std::size_t count) {
        cexpr::require(allocator);
        return static_cast<T*>(allocator->allocateUnmanaged(mem::type_info_of<T>, count));
    }

    bool operator == (const FrameAllocatorAdaptor& other) const {
        return allocator == other.allocator;
    }

    bool operator != (const FrameAllocatorAdaptor& other) const {
        return allocator != other.allocator;
    }

    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal = std::false_type;
};



template <typename T>
struct mem::allocator_traits<FrameAllocatorAdaptor<T>> {
    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::false_type;
    using is_always_equal = std::true_type;
};