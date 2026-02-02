#pragma once
#include <cstdint>
#include <iostream>

using EntityID = uint32_t;

class Entity {
    EntityID myID;
public:
    constexpr static EntityID ID_BITS  = 24;
    constexpr static EntityID GEN_BITS = 8;
    constexpr static EntityID ID_MASK  = (1u << ID_BITS) - 1;
    constexpr static EntityID GEN_MASK = ((1u << GEN_BITS) - 1) << ID_BITS;
    constexpr static EntityID GEN_SHIFT = ID_BITS;
    constexpr static EntityID MAX_GENERATION = (1u << GEN_BITS) - 1;
    constexpr static EntityID MAX_ID = ID_MASK;

    constexpr Entity() : myID(0) {}

    constexpr Entity(const EntityID gen, const EntityID id)
        : myID(gen << GEN_SHIFT | id & ID_MASK)
    {}

    static Entity max() {
        return Entity(MAX_GENERATION, MAX_ID);
    }

    EntityID id() const {
        return myID & ID_MASK;
    }

    EntityID gen() const {
        return myID >> GEN_SHIFT;
    }
    
    bool operator < (const Entity& other) const {
        return id() < other.id();
    }

    bool operator == (const Entity& other) const {
        return id() == other.id();
    }

    bool operator != (const Entity& other) const {
        return id() != other.id();
    }
    
    friend std::ostream& operator << (std::ostream& os, const Entity& e) {
        os << "Entity{" << e.gen() << ", " << e.id() << "}";
        return os;
    };

    operator bool() const {
        return id() != 0;
    }
};

constexpr static auto fib_const = 11400714819322319848ull;

template <> struct std::hash<Entity> {
    size_t operator () (const Entity e) const noexcept {
        return e.id() * fib_const;
    }
};

template <> struct std::hash<std::pair<Entity, Entity>> {
    size_t operator () (const std::pair<Entity, Entity>& e) const noexcept {
        const size_t h1 = std::hash<Entity>{}(e.first);
        const size_t h2 = std::hash<Entity>{}(e.second);

        return h1 ^ (h2 + fib_const + (h1 << 6) + (h1 >> 2));
    }
};

static constexpr auto NullEntity = Entity{0, 0};