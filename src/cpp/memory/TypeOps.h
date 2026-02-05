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

    inline void* allocate(typeindex type, const size_t count) {
        return operator new(type.size() * count, std::align_val_t(type.align()));
    }

    inline void deallocate(typeindex type, void* memory) {
        operator delete(memory, std::align_val_t(type.align()));
    }

    constexpr size_t aligned_offset(typeindex type, size_t byteOffset) {
        return (byteOffset + type.align() - 1) & ~(type.align() - 1);
    }

    constexpr size_t padding(typeindex type, size_t byteOffset) {
        return (byteOffset + type.align() - 1) & ~(type.align() - 1) - byteOffset;
    }

    constexpr size_t padding(size_t byteOffset, size_t alignment) {
        size_t aligned = (byteOffset + alignment - 1) & ~(alignment - 1);
        return aligned - byteOffset;
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
