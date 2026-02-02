#pragma once
#include "ECS/Entity/Entity.h"
#include "ECS/Entity/MetadataProvider.h"
#include "ECS/Forge/NameStagingBuffer.h"
#include "ECS/Level/LevelContext.h"
#include "ECS/Entity/EntityName.h"

class NameComponentType : public ComponentType<NameComponentType> {
    struct NameNode {
        EntityName name;
        size_t refCount = 0;
    };

    EntityMetadataStorage<EntityName> metadata;
    mem::free_list_allocator allocator;
    std::unordered_map<size_t, NameNode> names;

    NameStagingBuffer staging;
public:
    NameComponentType() : allocator(0.2 * 1024 * 1024) {}

    void onSynchronize(LevelContext& level);

    void onCreateEntity(const Entity& e, const EntityName &name) {
        staging.setName(e, name);
    }

    void onAddComponent(const Entity& e, const EntityName &name) {
        staging.setName(e, name);
    }

    template <typename T>
    void onRemoveComponent(const Entity& e);

    EntityName getName(const Entity& e) {
        return metadata[e];
    }
};

class NameQuery : public EntityComponentQuery {
    NameComponentType& storage;
public:
    explicit NameQuery(LevelContext& level);

    bool has(const Entity& e) const {
        return storage.getName(e).isNamed();
    }

    EntityName get(const Entity& e) const {
        return storage.getName(e);
    }
};