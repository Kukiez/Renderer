#include "BooleanArchetype.h"

BooleanArchetype::BooleanArchetype(BooleanArchetype &&other) noexcept
: entities(std::move(other.entities)), active(other.active) {
    other.active = false;
}

BooleanArchetype & BooleanArchetype::operator=(BooleanArchetype &&other) noexcept {
    if (this != &other) {
        entities = std::move(other.entities);
        active = other.active;
        other.active = false;
    }
    return *this;
}

void BooleanArchetype::add(const Entity* entities, const size_t count) {
    for (size_t i = 0; i < count; ++i) {
        this->entities.set_and_expand(entities[i].id());
    }
}

void BooleanArchetype::remove(const Entity* entities, const size_t count) {
    for (size_t i = 0; i < count; ++i) {
        this->entities.reset(entities[i].id());
    }
}

bool BooleanArchetype::has(const Entity& entity) {
    if (entities.size() < entity.id()) {
        return false;
    }
    return entities.test(entity.id());
}

void BooleanArchetype::reset() {
    entities = {};
}
