#pragma once
#include "BooleanArchetype.h"
#include <ECS/Component/ComponentMap.h>
#include <ECS/Entity/Entity.h>
#include <ECS/Entity/MetadataProvider.h>
#include <memory/free_list_allocator.h>

struct BooleanMetadata {
    mem::bitset<mem::free_list_adaptor<size_t>> tags;

    BooleanMetadata(mem::free_list_allocator* alloc) : tags(alloc) {}
};

class BooleanStorage {
    BooleanArchetype& getOrCreateArchetype(const TypeUUID type) {
        return archetypes.getOrCreate(type, !componentRegistry->getFieldOf(type)->isLookupOnly);
    }

    BooleanArchetype& getArchetype(const TypeUUID type) {
        return archetypes[type];
    }
    EntityMetadataStorage<BooleanMetadata>& metadata;
    ComponentMap<BooleanArchetype> archetypes;
    ComponentKindRegistry<BooleanField>* componentRegistry;
public:
    BooleanStorage(ComponentKindRegistry<BooleanField>* registry, auto& meta) : componentRegistry(registry), metadata(meta) {}

    void confirmArchetypeExists(const TypeUUID type) {
        getOrCreateArchetype(type);
    }

    void deleteEntity(const Entity& entity);

    void add(const Entity* entities, TypeUUID type, size_t count);

    void remove(const Entity* entities, TypeUUID type, size_t count);

    auto& getArchetypeUnchecked(const TypeUUID type) {
        return archetypes.getUnchecked(type);
    }

    bool has(const Entity& entity, const TypeUUID type) const {
        return metadata[entity].tags.test(type.id());
    }

    void reset() {
        for (auto& archetype : archetypes) {
            archetype.reset();
        }
    }
};