#pragma once
#include <unordered_map>
#include <string>
#include <string_view>

namespace mem {
    struct string_hash {
        using is_transparent = void;

        size_t operator () (const std::string_view view) const noexcept {
            return std::hash<std::string_view>{}(view);
        }
    };

    struct string_eq {
        using is_transparent = void;

        bool operator () (const std::string_view rhs, const std::string_view lhs) const noexcept {
            return rhs == lhs;
        }
    };

    template <typename Key, typename Alloc = std::allocator<std::pair<const std::string, Key>>>
    using unordered_stringmap = std::unordered_map<std::string, Key, string_hash, string_eq, Alloc>;

    template <typename SizeTRange>
    requires (std::ranges::range<SizeTRange> && requires(SizeTRange r) {
        { *r.begin() } -> std::convertible_to<size_t>;
    })
    size_t hash(SizeTRange range) {
        size_t hash = 14695981039346656037ull;
        for (const size_t v : range) {
            hash ^= v;
            hash *= 1099511628211ull; // FNV prime
        }
        return hash;
    }
}