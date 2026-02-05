#pragma once
#include <type_traits>
#include <concepts>
#include <constexpr/assert.h>
#include "type_info.h"
#include "TypeOps.h"

namespace mem {
    template <typename Alloc>
    struct allocator_traits {
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_move_assignment = std::false_type;
        using is_always_equal = std::true_type;
    };

    template <typename T, typename Alloc>
    concept IsAllocator = requires(Alloc alloc, T t) {
        {alloc.allocate(0) } -> std::same_as<T*>;
        {alloc.deallocate(&t, 0) } -> std::same_as<void>;
        {alloc.destroy(&t) } -> std::same_as<void>;
    };

    template <typename Alloc>
    concept IsDynamicAllocator = requires(Alloc alloc, typeindex type, void* mem) {
        { alloc.allocate(type, size_t(0)) } -> std::convertible_to<void*>;
        alloc.deallocate(type, mem, size_t(0));
        alloc.destroy(type, mem, size_t(0));

        []<typename T>(Alloc& alloc, T&& val) {
            alloc.construct<std::decay_t<T>>(type_info_of<std::decay_t<T>>, (void*)(0), std::forward<T>(val));
        }(alloc, int(5));

        alloc.move_construct(type, mem, mem, size_t(0));
        alloc.copy_construct(type, mem, mem, size_t(0));          
    };

    template <typename Alloc>
    concept IsShrinkableAllocator = requires(Alloc alloc) {
        alloc.shrink_alloc((void*)0, size_t(0), size_t(0));
        /* alloc.shrink_alloc(memory, oldSize, newSize) */
    };

    template <typename Alloc>
    concept IsAlwaysEqualAllocator = allocator_traits<Alloc>::is_always_equal::value;

    template <typename Alloc>
    concept ShouldPropagateOnCopy = allocator_traits<Alloc>::propagate_on_container_copy_assignment::value;

    template <typename Alloc>
    concept ShouldPropagateOnMove = allocator_traits<Alloc>::propagate_on_container_move_assignment::value;

    using default_allocator_traits = allocator_traits<void>;

    template <typename T>
    struct default_destroy {
        static void destroy(T* ptr) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                ptr->~T();
            }
        }
    };

    enum class alignment : size_t {};
    
    template <typename T>
    struct default_construct {
        template <typename... Args>
        static T* construct(T* ptr, Args&&... args) {
            return new (ptr) T(std::forward<Args>(args)...);
        }
    };

    struct same_alloc_schema {
        static size_t grow(const size_t current, const size_t required) {
            return current + required;
        }
    };

    inline void* raw_alloc(size_t count, size_t align) {
        return operator new(count, static_cast<std::align_val_t>(align));
    }

    inline void raw_delete(void* ptr, size_t align) {
        operator delete(ptr, static_cast<std::align_val_t>(align));
    }

    struct no_deallocate_fn {};

    template <typename T>
    using SelectInputPtr = std::conditional_t<std::is_void_v<T>, void, T>;

    template <typename T, typename... Args,
        typename Allocator, typename SizeType,
        typename AllocateFn = decltype([](Allocator& allocator, const size_t bytes, const size_t alignment) { return allocator.allocate(bytes, alignment); }),
        typename DeallocateFn = decltype([](Allocator& allocator, void* ptr, const size_t bytes, const size_t alignment) { allocator.deallocate(ptr, bytes, alignment); })
    >
    requires std::constructible_from<T, Args...>
    T* emplace_back(Allocator& allocator, T*& arrayPtr, SizeType& numElements, SizeType& numCapacity, const SizeType newCapacity, AllocateFn&& allocFn, DeallocateFn&& deallocateFn, Args&&... args) {
        if (!arrayPtr) {
            arrayPtr = (T*)allocFn(allocator, sizeof(T) * newCapacity, alignof(T));
            numCapacity = newCapacity;
        } else if (numElements == numCapacity) {
            T* newArr = (T*)allocFn(allocator, sizeof(T) * newCapacity, alignof(T));

            for (SizeType i = 0; i < numElements; ++i) {
                new (newArr + i) T(std::move(arrayPtr[i]));
                arrayPtr[i].~T();
            }

            if constexpr (std::is_same_v<DeallocateFn, no_deallocate_fn>) {

            } else {
                deallocateFn(allocator, arrayPtr, numElements * sizeof(T), alignof(T));
            }

            arrayPtr = newArr;
            numCapacity = newCapacity;
        }
        return new (arrayPtr + numElements++) T(std::forward<Args>(args)...);
    }

    template <typename T>
    struct default_allocator {
        static T* allocate(const size_t count) {
            return static_cast<T*>(operator new(
                count * sizeof(T), static_cast<std::align_val_t>(alignof(T))
            ));
        }

        static void deallocate(T* ptr, const size_t) {
            operator delete(
                ptr, static_cast<std::align_val_t>(alignof(T))
            );
        }

        template <typename... Args>
        requires std::constructible_from<T, Args...>
        static T* construct(T* ptr,  Args&&... args) {
            return new (ptr) T(std::forward<Args>(args)...);     
        }

        static void destroy(T* ptr) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                ptr->~T();
            }
        }

        static void move_range(T* dst, T* src, const size_t count) {
            memcpy(dst, src, sizeof(T) * count);
        }
    };

    template <typename T>
    struct allocator_traits<default_allocator<T>> {
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_move_assignment = std::false_type;
        using is_always_equal = std::true_type;
    };

    struct dynamic_allocator {
        void* allocate(typeindex type, size_t count) {
            return raw_alloc(type.size() * count, type.align());
        }

        void deallocate(typeindex type, void* mem, size_t count) {
            raw_delete(mem, type.align());
        }

        template <typename T, typename... Args>
        void construct(typeindex type, void* mem, Args&&... args) {
            new (mem) T(std::forward<Args>(args)...);
        }

        void destroy(typeindex type, void* mem, size_t count) {
            type.destroy(mem, count);
        }

        void copy_construct(typeindex type, void* dst, const void* src, size_t count) {
            type.copy(dst, src, count);
        }

        void move_construct(typeindex type, void* dst, void* src, size_t count) {
            type.move(dst, src, count);
        }
    };

    template <>
    struct allocator_traits<dynamic_allocator> {
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_move_assignment = std::false_type;
        using is_always_equal = std::true_type;
    };

    template <size_t Size = 8, size_t Align = 8>
    struct small_buffer_dynamic_allocator : dynamic_allocator {
        alignas(Align) char buf[Size];

        void* allocate(typeindex type, size_t count) {
            if (type.size() <= Size && type.align() <= Align) {
                return buf;
            } else {
                return raw_alloc(type.size() * count, type.align());
            }
        }

        void deallocate(typeindex type, void* mem, size_t count) {
            if (mem != buf) {
                raw_delete(mem, type.align());
            }
        }
    };

    template <size_t S, size_t A>
    struct allocator_traits<small_buffer_dynamic_allocator<S, A>> {
        using is_always_equal = std::false_type;
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_move_assignment = std::false_type;
    };

    struct doubling_schema {
        static size_t grow(size_t current, size_t required) {
            return (current * 2) + required;
        }
    };

    template <typename Alloc>
    auto copy_forward_allocator(const Alloc& src) {
        using DecayedAlloc = std::decay_t<Alloc>;

        if constexpr (IsAlwaysEqualAllocator<DecayedAlloc> || ShouldPropagateOnCopy<DecayedAlloc>) {
            return DecayedAlloc(src);
        } else if constexpr (std::is_default_constructible_v<DecayedAlloc>) {
            return DecayedAlloc{};
        } else {
            static_assert(false, "Allocator cannot be copied or default constructed");
        }
    }

    template <typename Alloc>
    void copy_assign_allocator(Alloc& dst, const Alloc& src) {
        using DecayedAlloc = std::decay_t<Alloc>;

        if constexpr (IsAlwaysEqualAllocator<DecayedAlloc> || ShouldPropagateOnCopy<DecayedAlloc>) {
            dst = src;
        }
    }

    template <typename Alloc>
    auto move_forward_allocator(Alloc& src) {
        using DecayedAlloc = std::decay_t<Alloc>;

        if constexpr (IsAlwaysEqualAllocator<DecayedAlloc>) {
            return src;
        } else if constexpr (ShouldPropagateOnMove<DecayedAlloc>) {
            return DecayedAlloc(std::move(src));
        } else if constexpr (std::is_default_constructible_v<DecayedAlloc>) {
            return DecayedAlloc{};
        } else {
            static_assert(false, "Allocator cannot be moved or default constructed");
        }
    }

    template <typename Alloc>
    void move_assign_allocator(Alloc& dst, Alloc& src) {
        using DecayedAlloc = std::decay_t<Alloc>;

        if constexpr (ShouldPropagateOnMove<DecayedAlloc>) {
            dst = std::move(src);
        }
    }

    template <typename T>
    struct in_place_t {};
}