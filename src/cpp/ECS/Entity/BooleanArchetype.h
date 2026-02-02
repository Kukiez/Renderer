#pragma once
#include "Entity.h"
#include "memory/bitset.h"

class BooleanArchetype {
    mem::bitset<> entities;

    bool active = false;
public:
    BooleanArchetype() = default;
    BooleanArchetype(const bool active) : active(active) {}

    BooleanArchetype(const BooleanArchetype&) = delete;
    BooleanArchetype& operator = (const BooleanArchetype&) = delete;

    BooleanArchetype(BooleanArchetype&& other) noexcept;
    BooleanArchetype& operator = (BooleanArchetype&& other) noexcept;

    ~BooleanArchetype() = default;

    void add(const Entity* entities, size_t count);

    void remove(const Entity* entities, size_t count);

    bool has(const Entity& entity);

    bool isActive() const {
        return active;
    }

    void reset();
};
