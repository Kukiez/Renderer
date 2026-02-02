#pragma once
#include "type_info.h"

namespace mem {
    inline void memcpy(void* dst, void* src, const size_t bytes) {
        std::memcpy(dst, src, bytes);
    }

    inline void default_memcpy(void* dst, const void* src, const size_t bytes) {
        std::memcpy(dst, src, bytes);
    }

    constexpr char* char_cast(void* ptr) {
        return static_cast<char*>(ptr);
    }

    constexpr size_t align_of(const type_info* type) {
        return type->align;
    }

    constexpr size_t size_of(const type_info* type) {
        return type->size;
    }

    constexpr size_t hash_of(const type_info* type) {
        return type->hash;
    }

    constexpr const char* name_of(const type_info* type) {
        return type->name;
    }

    constexpr size_t stride(const type_info* type) {
        return (type->size + type->align - 1) & ~(type->align - 1);
    }

    constexpr void* offset(const type_info* type, void* memory, const size_t index) {
        char* ptr = char_cast(memory);
        return ptr + stride(type) * index;
    }


    inline void destroy_at(const type_info* type, void* memory, const size_t count = 1) {
        if (type->destruct)
            type->destruct(memory, count);
    }

    inline void* allocate(const type_info* type, const size_t count) {
        return operator new(type->size * count, std::align_val_t(type->align));
    }

    inline void deallocate(const type_info* type, void* memory) {
        operator delete(memory, std::align_val_t(type->align));
    }

    inline void move(const type_info* type, void* dst, void* src, const size_t count = 1) {
        if (type->move)
            type->move(dst, src, count);
        else
            default_memcpy(dst, src, type->size * count);
    }

    inline void copy(const type_info* type, void* dst, const void* src, const size_t count = 1) {
        if (type->copy)
            type->copy(dst, src, count);
        else
            default_memcpy(dst, src, type->size * count);
    }

    inline void swap(const type_info* type, void* first, void* second) {
        type->swap(first, second);
    }

    constexpr size_t aligned_offset(const type_info* type, size_t byteOffset) {
        return (byteOffset + type->align - 1) & ~(type->align - 1);
    }

    constexpr size_t padding(const type_info* type, size_t byteOffset) {
        return (byteOffset + type->align - 1) & ~(type->align - 1) - byteOffset;
    }

    constexpr size_t padding(size_t byteOffset, size_t alignment) {
        size_t aligned = (byteOffset + alignment - 1) & ~(alignment - 1);
        return aligned - byteOffset;
    }

    constexpr bool fits_in_address_of_type(const type_info* existingType, const type_info* newType) {
        return (newType->size <= existingType->size && newType->align <= existingType->align);
    }

    template <int N, typename T>
    constexpr T round_to(T n) {
        static_assert(std::is_integral_v<T>, "T must be an integral type");
        using U = std::make_unsigned_t<T>;
        U u = static_cast<U>(n);
        if constexpr (std::numeric_limits<U>::max() > U(0)) {
            if (u > std::numeric_limits<U>::max() - U(N - 1)) {
                return static_cast<T>(u);
            }
        }
        return static_cast<T>((u + U(N - 1)) & ~U(N - 1));

    }
    template <typename T>
    constexpr T round_up_to_64(T n) {
        static_assert(std::is_integral_v<T>, "T must be an integral type");
        using U = std::make_unsigned_t<T>;
        U u = static_cast<U>(n);
        if constexpr (std::numeric_limits<U>::max() > U(0)) {
            if (u > std::numeric_limits<U>::max() - U(63)) {
                return static_cast<T>(u);
            }
        }
        return static_cast<T>((u + U(63)) & ~U(63));
    }
}
