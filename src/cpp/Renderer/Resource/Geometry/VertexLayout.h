#pragma once
#include <constexpr/TypeInfo.h>
#include <memory/Span.h>

#include "AttributeTraits.h"

class VertexLayout {
    const AttributeInfo* attributes{};
    size_t numLocations{};
public:
    constexpr VertexLayout() = default;

    constexpr VertexLayout(const AttributeInfo* attributes, size_t count) : attributes(attributes), numLocations(count) {}

    auto getAttributes() const {
        return mem::make_range(attributes, numLocations);
    }

    size_t size() const {
        return numLocations;
    }
};

class VertexLayoutHash {
    size_t hash = 0;
public:
    constexpr VertexLayoutHash() = default;

    template <typename... Attrs>
    requires (std::is_same_v<std::decay_t<Attrs>, AttributeInfo> && ...)
    constexpr VertexLayoutHash(Attrs&&... attrs) {
        (add(attrs), ...);
    }

    bool operator==(const VertexLayoutHash& other) const { return hash == other.hash; }

    constexpr void add(const AttributeInfo &info) {
        hash ^= cexpr::hash(static_cast<size_t>(info.type)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }

    constexpr size_t get() const { return hash; }

};
template <auto... Attrs>
struct StaticVertexLayout {
    static constexpr auto Attributes = std::array{
        Attrs.Info...
    };
    static constexpr VertexLayout Layout = VertexLayout(Attributes.data(), sizeof...(Attrs));
    static constexpr VertexLayoutHash Hash{Attrs.Info...};
};

template <typename... Attrs>
constexpr auto VertexLayoutOf(Attrs...) {
    return StaticVertexLayout<Attrs{}...>{};
}