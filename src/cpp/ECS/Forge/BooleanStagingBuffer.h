#pragma once
#include "ECS/ThreadLocal.h"
#include "ECS/Component/ComponentMap.h"
#include "ECS/Entity/Entity.h"
#include "memory/alloc.h"
#include "memory/byte_arena.h"
#include "memory/vector.h"

class BooleanStagingBuffer {
    using Arena = mem::byte_arena<mem::same_alloc_schema, 64>;
    struct Ops {
        struct Buffer {
            mem::vector<Entity, Arena::Adaptor<Entity>> newEntities;
            mem::vector<Entity, Arena::Adaptor<Entity>> removedEntities;

            Buffer() = default;

            explicit Buffer(Arena& arena) : newEntities(&arena), removedEntities(&arena) {}
        };
        Arena arena;
        ComponentMap<Buffer> changes;
    };
    ComponentKindRegistry<BooleanField>* componentRegistry{};
    ThreadLocal<Ops> ops;
public:
    explicit BooleanStagingBuffer(ComponentKindRegistry<BooleanField>* registry) : componentRegistry(registry) {}

    template <typename T>
    void add(const Entity& entity, T&& component) {
        const auto type = componentRegistry->getTypeID<T>();
        ops.local().changes.getOrCreate(type, ops.local().arena).newEntities.emplace_back(entity);
    }

    template <typename T>
    void remove(const Entity& entity) {
        const auto type = componentRegistry->getTypeID<T>();
        ops.local().changes.getOrCreate(type, ops.local().arena).removedEntities.emplace_back(entity);
    }

    void reset() {
        for (auto& [arena, changes] : ops) {
            for (auto& buffer : changes) {
                buffer.newEntities.release();
                buffer.removedEntities.release();
            }
            arena.reset_compact();
        }
    }

    auto begin() {
        return ops.begin();
    }

    auto end() {
        return ops.end();
    }
};
