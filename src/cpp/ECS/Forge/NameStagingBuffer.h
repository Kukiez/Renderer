#pragma once
#include "ECS/ThreadLocal.h"
#include "ECS/Entity/Entity.h"
#include "ECS/Entity/EntityName.h"
#include <memory/byte_arena.h>
#include <memory/vector.h>

class NameStagingBuffer {
    using Arena = mem::byte_arena<mem::same_alloc_schema, 64>;
    using NameVector = mem::vector<std::pair<Entity, EntityName>, Arena::Adaptor<std::pair<Entity, EntityName>>>;

    struct EntityNameChangeOps {
        Arena allocator = Arena(0.05 * 1024 * 1024);
        NameVector nameChanges;

        EntityNameChangeOps() : nameChanges(&allocator) {}

        void add(Entity entity, const EntityName& name);
    };

    ThreadLocal<EntityNameChangeOps> nameChangeOps;
public:
    void setName(Entity entity, const EntityName &name);

    void reset();

    auto begin();

    auto end();
};