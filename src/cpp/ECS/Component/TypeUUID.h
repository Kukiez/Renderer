#pragma once
#include "ComponentKind.h"

class TypeUUID {
    ComponentKind myKind{};
    uint16_t myIndex{};

    TypeUUID(const ComponentKind kind, const uint16_t index) : myKind(kind), myIndex(index) {}
public:
    TypeUUID() = default;

    static TypeUUID of(ComponentKind kind, uint16_t index) {
        return TypeUUID{kind, index};
    }

    ComponentKind kind() const {
        return myKind;
    }

    uint16_t id() const {
        return myIndex;
    }

    bool operator==(const TypeUUID other) const {
        return myKind == other.myKind && myIndex == other.myIndex;
    }

    bool operator!=(const TypeUUID other) const {
        return !(*this == other);
    }

    bool operator<(const TypeUUID other) const {
        return myKind < other.myKind || (myKind == other.myKind && myIndex < other.myIndex);
    }

    bool operator>(const TypeUUID other) const {
        return myKind > other.myKind || (myKind == other.myKind && myIndex > other.myIndex);
    }

    bool operator<=(const TypeUUID other) const {
        return !(*this > other);
    }

    bool operator>=(const TypeUUID other) const {
        return !(*this < other);
    }

    explicit operator uint32_t() const {
        return (static_cast<uint32_t>(myKind) << 16) | myIndex;
    }
};


template <>
struct std::hash<TypeUUID> {
    size_t operator()(const TypeUUID& type) const noexcept {
        size_t h1 = std::hash<ComponentKind>{}(type.kind());
        size_t h2 = std::hash<uint16_t>{}(type.id());
        return h1 ^ h2 << 1;
    }
};
