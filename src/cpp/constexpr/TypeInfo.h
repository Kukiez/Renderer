#pragma once
#include <array>

#include "General.h"
#include "PackIteration.h"

namespace cexpr {
    constexpr size_t& hash_combine(std::size_t& seed, std::size_t value) {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }

    namespace detail {
        template <typename T>
        constexpr auto get_typename() {
            constexpr auto str = __FUNCSIG__;
            constexpr auto total = strlen(__FUNCSIG__);

            static constexpr auto name = [&]{
                constexpr size_t offset = strlen("auto __cdecl cexpr::detail::get_typename<");
                constexpr size_t backOffset = strlen(">(void)");

                constexpr size_t finalOffset = [&] {
                    if constexpr (std::is_enum_v<T>) {
                        return offset + strlen("enum ");
                    } else if constexpr (std::is_class_v<T>) {
                        if (str[offset] == 'c') {
                            return offset + strlen("class ");
                        } else {
                            return offset + strlen("struct ");
                        }
                    } else if constexpr (std::is_union_v<T>) {
                        return offset + strlen("union ");
                    } else {
                        return offset;
                    }
                }();

                std::array<char, total - finalOffset> name{};

                for (int i = 0; i < total - finalOffset - backOffset; ++i) {
                    name[i] = str[finalOffset + i];
                }
                return name;
            }();
            return name.data();
        }

        template <typename Enum, auto N>
        constexpr auto get_enumname() {
            constexpr auto str = __FUNCSIG__;
            constexpr auto strLen = strlen(__FUNCSIG__);

            static constexpr auto name = [&]{
                constexpr size_t baseOffset =
                    strlen("auto __cdecl cexpr::detail::get_enumname<")
                    + strlen("enum ");

                constexpr size_t offset = [&]{
                    for (size_t i = strLen; i >= 0; --i) {
                        if (str[i] == ':') {
                            return i + 1;
                        } else if (i <= baseOffset) {
                            return baseOffset;
                        }
                    }
                }();
                constexpr size_t backOffset = strlen(">(void)");

                std::array<char, strLen - offset> name{};

                for (int i = 0; i < strLen - offset - backOffset; ++i) {
                    name[i] = str[offset + i];
                }
                return name;
            }();
            return name.data();
        }

        template <typename T>
        consteval size_t type_hash() {
            constexpr auto name = get_typename<T>();

            constexpr size_t fnv_prime = 1099511628211ull;

            size_t hash = 14695981039346656037ull;
            for (const char* c = name; *c != '\0'; ++c) {
                hash ^= static_cast<size_t>(*c);
                hash *= fnv_prime;
            }
            return hash;
        }

        template <typename... Ts>
        consteval size_t pack_hash() {
            size_t seed = 0;
            (hash_combine(seed, detail::type_hash<Ts>()), ...);
            return seed;
        }
    }


    template <typename T>
    T* declpointer() {
        return nullptr;
    }

    template <typename T>
    T declval() {
        return *declpointer<T>();
    }

    template <typename T>
    constexpr auto type_hash_v = detail::type_hash<T>();

    template <typename T>
    constexpr const char* name_of = detail::get_typename<T>();

    template <typename T, size_t N>
    constexpr const char* enum_name_v = detail::get_enumname<T, T{N}>();

    template <typename T>
    requires std::is_enum_v<T>
    constexpr std::underlying_type_t<T> enum_cast(T val) {
        return static_cast<std::underlying_type_t<T>>(val);
    }

    constexpr size_t type_hash(const char* str) {
        size_t hash = 14695981039346656037ull;
        for (const char* c = str; *c != '\0'; ++c) {
            constexpr size_t fnv_prime = 1099511628211ull;
            hash ^= static_cast<size_t>(*c);
            hash *= fnv_prime;
        }
        return hash;
    }

    constexpr size_t type_hash(const char* str, const size_t length) {
        size_t hash = 14695981039346656037ull;
        for (size_t i = 0; i < length; ++i) {
            constexpr size_t fnv_prime = 1099511628211ull;
            hash ^= static_cast<size_t>(str[i]);
            hash *= fnv_prime;
        }
        return hash;
    }

    template <std::integral T>
    constexpr std::size_t hash(T val) noexcept {
        size_t v = static_cast<size_t>(val);
        v += 0x9e3779b97f4a7c15ull;
        v = (v ^ (v >> 30)) * 0xbf58476d1ce4e5b9ull;
        v = (v ^ (v >> 27)) * 0x94d049bb133111ebull;
        return v ^ (v >> 31);
    }

    template <int N>
    constexpr size_t type_hash(const char (&str)[N]) {
        auto hash = type_hash(str);
        return hash;
    }
}
