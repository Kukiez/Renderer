#pragma once
#include <cstdint>
#include <xhash>

class ComponentKind {
    uint16_t myID = 0;
public:
    ComponentKind() = default;

    explicit ComponentKind(const uint16_t id) : myID(id) {}

    explicit operator unsigned() const {
        return myID;
    }

    explicit operator uint16_t() const {
        return myID;
    }

    explicit operator int() const {
        return myID;
    }

    bool operator == (const ComponentKind& other) const {
        return myID == other.myID;
    }

    bool operator != (const ComponentKind& other) const {
        return myID != other.myID;
    }

    bool operator < (const ComponentKind& other) const {
        return myID < other.myID;
    }

    bool operator > (const ComponentKind& other) const {
        return myID > other.myID;
    }

    uint16_t id() const {
        return myID;
    }

    template <typename T>
    static ComponentKind of();

    template <typename T>
    bool is() const;
};

template <> struct std::hash<ComponentKind> {
    size_t operator()(const ComponentKind& kind) const noexcept {
        return std::hash<uint16_t>{}(kind.id());
    }
};