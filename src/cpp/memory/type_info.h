#pragma once
#include <constexpr/TypeInfo.h>
#include <iostream>

#include "constexpr/assert.h"

namespace mem {
    class Typename {
        const char* name = nullptr;
    public:
        Typename() = default;
        Typename(const char* name) : name(name) {}

        operator const char*() const { return name; }

        friend std::ostream& operator << (std::ostream& os, const Typename& tn) {
            switch (tn[0]) {
                case 'c': os << tn.name + strlen("class "); return os;
                case 'e': os << tn.name + strlen("enum "); return os;
                case 's': os << tn.name + strlen("struct "); return os;
                case 'u': os << tn.name + strlen("union "); return os;
                default: return os << tn.name;
            }
        }
    };

    struct TypeHash {
        std::array<uint64_t,2> hash;

        constexpr static std::array<uint64_t, 2> fnv1a128(const char* str) {
            std::array<uint64_t,2> hash = { 0xcbf29ce484222325ULL, 0xcbf29ce484222325ULL };
            for (size_t i = 0; str[i]; ++i) {
                hash[0] ^= static_cast<uint64_t>(str[i]);
                hash[0] *= 0x100000001b3ULL;
                hash[1] ^= static_cast<uint64_t>(str[i]);
                hash[1] *= 0x100000001b3ULL;
            }
            return hash;
        }

        template <typename T>
        static constexpr TypeHash of() {
            using DT = std::decay_t<T>;
            return TypeHash{fnv1a128(cexpr::name_of<DT>)};
        }

        constexpr bool operator == (const TypeHash& other) const {
            return hash == other.hash;
        }

        constexpr bool operator != (const TypeHash& other) const {
            return hash != other.hash;
        }

        constexpr bool operator < (const TypeHash& other) const {
            return hash < other.hash;
        }

        constexpr bool operator > (const TypeHash& other) const {
            return hash > other.hash;
        }

        friend std::ostream& operator << (std::ostream& os, const TypeHash& hash) {
            return os << hash.hash[0] << hash.hash[1];

        }
    };

    using DestructorFn = void(*)(void* mem, size_t count);
    using CopyFn = void(*)(void* dst, const void* src, size_t count);
    using MoveFn = void(*)(void* dst, void* src, size_t count);
    using SwapFn = void(*)(void* first, void* second);

    struct type_info {
        friend class typeindex;
    private:
        const char* name = 0;
        size_t hash = 0;
        TypeHash typeHash{};
        size_t size = 0;
        size_t align = 0;

        CopyFn copy = 0;
        MoveFn move = 0;
        DestructorFn destruct = 0;
        SwapFn swap = 0;
    public:
        constexpr bool is_trivially_destructible() const {
            return destruct == nullptr;
        }

        constexpr bool operator == (const type_info& other) const {
            return name == other.name;
        }

        constexpr bool operator != (const type_info& other) const {
            return name != other.name;
        }

        template <typename Type>
        static constexpr const type_info* of() {
            using T = std::decay_t<Type>;

            static constexpr auto type = []{
                type_info type;

                type.typeHash = TypeHash::of<T>();
                if constexpr (std::is_same_v<T, void>) {
                    type.name = "void";
                    type.hash = 0;
                    type.size = 0;
                    type.align = 0;
                } else {
                    type.name = cexpr::name_of<T>;
                    type.hash = cexpr::type_hash_v<T>;
                    type.size = sizeof(T);
                    type.align = alignof(T);


                    type.swap = [](void* first, void* second) {
                        T* fT = static_cast<T*>(first);
                        T* sT = static_cast<T*>(second);

                        if constexpr (std::swappable<T>) {
                            std::swap(*sT, *fT);
                        } else {
                            assert(false);
                        }
                    };

                    if constexpr (!std::is_trivially_copyable_v<T>) {
                        type.copy = [](void* dst, const void* src, size_t count) {
                            T* dstT = static_cast<T*>(dst);
                            const T* srcT = static_cast<const T*>(src);

                            if constexpr (std::is_copy_assignable_v<T>) {
                                for (size_t i = 0; i < count; ++i) {
                                    dstT[i] = srcT[i];
                                }
                            } else {
                                cexpr::require(false);
                            //    static_assert(false, "T is not Copy Assignable");
                            }
                        };
                    }

                    if constexpr (!std::is_trivially_move_constructible_v<T>) {
                        type.move = [](void* dst, void* src, size_t count) {
                            T* dstT = static_cast<T*>(dst);
                            T* srcT = static_cast<T*>(src);

                            if constexpr (std::is_move_assignable_v<T>) {
                                for (int i = 0; i < count; ++i) {
                                    new (dstT + i) T(std::move(srcT[i]));
                                }
                            } else {
                                assert(false);
                            //    static_assert(false, "T is not Move Assignable");
                            }
                        };
                    }

                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        type.destruct = [](void* mem, size_t count) {
                            T* t = static_cast<T*>(mem);

                            for (int i = 0; i < count; ++i) {
                                (t + i)->~T();
                            }
                        };
                    }
                }
                return type;
            }();
            return &type;
        }
    };

    class typeindex {
        const type_info* type;
    public:
        constexpr typeindex() : type(type_info::of<void>()) {}
        constexpr typeindex(const type_info* typeinfo) : type(typeinfo) {}

        constexpr size_t hash() const { return type->hash; }
        constexpr const char* name() const { return type->name; }
        TypeHash type_hash() const { return type->typeHash; }
        constexpr size_t size() const { return type->size; }
        constexpr size_t align() const { return type->align; }

        constexpr bool is_trivially_destructible() const { return type->is_trivially_destructible(); }
        constexpr bool is_trivially_copyable() const { return type->copy == nullptr; }
        constexpr bool is_trivially_moveable() const { return type->move == nullptr; }

        constexpr bool operator == (const typeindex& other) const { return type->typeHash == other.type->typeHash; }
        constexpr bool operator != (const typeindex& other) const { return type->typeHash != other.type->typeHash; }

        constexpr bool operator > (const typeindex& other) const { return type->typeHash > other.type->typeHash; }
        constexpr bool operator < (const typeindex& other) const { return type->typeHash < other.type->typeHash; }

        constexpr DestructorFn destructor() const { return type->destruct; }
        constexpr CopyFn copy() const { return type->copy; }
        constexpr MoveFn move() const { return type->move; }
        constexpr SwapFn swap() const { return type->swap; }

        constexpr void destroy(void* mem, size_t count = 1) const { type->destruct(mem, count); }

        constexpr void copy(void* dst, const void* src, size_t count = 1) const {
            if (is_trivially_copyable()) std::memcpy(dst, src, type->size * count);
            else type->copy(dst, src, count);
        }

        constexpr void move(void* dst, void* src, size_t count = 1) const {
            if (is_trivially_moveable()) std::memcpy(dst, src, type->size * count);
            else type->move(dst, src, count);
        }

        constexpr void swap(void* first, void* second) const {
            type->swap(first, second);
        }

        size_t stride() const {
            return (type->size + type->align - 1) & ~(type->align - 1);
        }

        void* index(void* memory, size_t index) const {
            return static_cast<char*>(memory) + stride() * index;
        }

        const void* index(const void* memory, size_t index) const {
            return static_cast<const char*>(memory) + stride() * index;
        }

        template <typename T>
        bool is() const {
            return type->typeHash == type_info::of<T>()->typeHash;
        }
    };

    class default_constructor {
        using ConstructorFn = void(*)(void* mem, size_t count);

        ConstructorFn constructor = nullptr;

        default_constructor(ConstructorFn constructor) : constructor(constructor) {}
    public:
        default_constructor() = default;

        void* construct(void* adr, const size_t count) const {
            constructor(adr, count);
            return adr;
        }

        bool operator () (void* adr, const size_t count) const {
            return try_construct(adr, count);
        }

        bool try_construct(void* adr, const size_t count) const {
            if (constructor) {
                constructor(adr, count);
                return true;
            }
            return false;
        }

        template <typename T>
        static void construct_at(void* adr, size_t count) {
            for (int i = 0; i < count; ++i) {
                new (static_cast<T*>(adr) + i) T();
            }
        }

        template <typename T>
        requires std::is_default_constructible_v<T>
        static constexpr default_constructor of() {
            return default_constructor{ construct_at<T> };
        }
    };
    template <typename T>
    constexpr static typeindex type_info_of = {type_info::of<T>()};

    template <size_t Align>
    struct alignas(Align) aligned_char_t {
        char c;
    };
}
