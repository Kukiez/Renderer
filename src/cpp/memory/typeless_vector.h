#pragma once
#include "type_info.h"
#include "alloc.h"
#include "TypeOps.h"
#include "vector.h"

namespace mem {
    struct type_erased_allocator {
        const mem::type_info* type;

        char* allocate(const size_t count) {
            return new char[type->size * count];
        }

        void deallocate(char* ptr, size_t) {
            delete[] ptr;
        }

        template <typename T, typename... Args>
        T* construct_typed(void* buffer, const size_t index, Args&&... args) {
            return new ((T*)buffer + index) T(std::forward<Args>(args)...);
        }

        void construct_typeless(void* dst, void* src) {
            memcpy(dst, src, type->size);
        }

        void destroy(void* buf, size_t count) {
            mem::destroy_at(type, buf, count);
        }
    };

    template <
        typename Alloc = type_erased_allocator,
        typename ReallocSchema = doubling_schema,
        typename size_type = size_t
    >
    class byte_vector {
        bool has_memory(const size_type required) {
            return current + required < max;
        }
        
        void realloc(const size_type newCapacity) {
            char* newPtr = allocator.allocate(newCapacity);

            for (size_type i = 0; i < current; ++i) {
                allocator.construct_typeless(newPtr + i, ptr);
            }
            allocator.deallocate(ptr, current);
            ptr = newPtr;
            max = newCapacity;
        }
        
        void ensure_has_memory(const size_type required) {
            if (!has_memory(required)) {
                const size_type newCapacity = static_cast<size_type>(reallocSchema.grow(max, required));
                realloc(newCapacity);
            }
        }
        
        char* ptr = nullptr;
        Alloc allocator;
        ReallocSchema reallocSchema;
        size_type max = 0;
        size_type current = 0;
    public:
        byte_vector() = default;
        
        template <typename TAlloc>
        requires std::constructible_from<Alloc, TAlloc>
        byte_vector(TAlloc&& alloc) : allocator(std::forward<TAlloc>(alloc)) {}

        template <typename T, typename... Args>
        T& emplace_back(Args&&... args) {
            ensure_has_memory(1);
            return *allocator.construct_typed<T>(ptr, current++, std::forward<Args>(args)...);
        }

        template <typename T>
        T* data() {
            return reinterpret_cast<T*>(ptr);
        }


        template <typename T>
        vector_iterator_holder<T> iterator() {
            return {
                (T*)ptr, (T*)ptr + current
            };
        }

        void clear() {
            allocator.destroy(ptr, current);
            current = 0;
        }
    };
}