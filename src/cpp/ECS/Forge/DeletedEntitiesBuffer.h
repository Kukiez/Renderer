#pragma once
#include "ECS/ThreadLocal.h"
#include "ECS/Entity/Entity.h"

class DeletedEntities {
    const std::vector<Entity>& entities;
public:
    DeletedEntities(const std::vector<Entity>& entities) : entities(entities) {}

    auto begin() const {
        return entities.begin();
    }

    auto end() const {
        return entities.end();
    }

    std::vector<Entity> clone() const {
        std::vector<Entity> out;
        out.reserve(entities.size());
        out.insert(out.end(), entities.begin(), entities.end());
        return out;
    }
};

class DeletedEntitiesBuffer {
    ThreadLocal<std::vector<Entity>> deletedEntities;
public:
    DeletedEntitiesBuffer() = default;
    DeletedEntitiesBuffer(const DeletedEntitiesBuffer&) = delete;
    DeletedEntitiesBuffer& operator=(const DeletedEntitiesBuffer&) = delete;
    DeletedEntitiesBuffer(DeletedEntitiesBuffer&&) = delete;
    DeletedEntitiesBuffer& operator=(DeletedEntitiesBuffer&&) = delete;

    ~DeletedEntitiesBuffer() = default;

    void deleteEntity(const Entity& entity) {
        deletedEntities.local().emplace_back(entity);
    }

    void reset() {
        for (auto& ops : deletedEntities) {
            ops.clear();
        }
    }

    std::vector<Entity>& merge(std::vector<Entity>& out) {
        for (auto& ops : deletedEntities) {
            out.insert(out.end(), ops.begin(), ops.end());
        }
        return out;
    }
};
