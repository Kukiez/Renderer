#pragma once
#include "Entity.h"
#include "ECS/ECSAPI.h"
#include "memory/bitset.h"

class BooleanArchetype {
    mem::bitset<> entities;

    bool active = false;
public:
    BooleanArchetype() = default;
    BooleanArchetype(const bool active) : active(active) {}

    BooleanArchetype(const BooleanArchetype&) = delete;
    BooleanArchetype& operator = (const BooleanArchetype&) = delete;

    ECSAPI BooleanArchetype(BooleanArchetype&& other) noexcept;
    ECSAPI BooleanArchetype& operator = (BooleanArchetype&& other) noexcept;

    ~BooleanArchetype() = default;

    ECSAPI void add(const Entity* entities, size_t count);

    ECSAPI void remove(const Entity* entities, size_t count);

    ECSAPI bool has(const Entity& entity);

    bool isActive() const {
        return active;
    }

    ECSAPI void reset();
};
