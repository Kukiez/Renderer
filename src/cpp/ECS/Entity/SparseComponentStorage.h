#pragma once
#include <ECS/Component/ComponentMap.h>
#include "SecondaryArchetype.h"
#include <ECS/Component/ComponentRegistry.h>
#include <ECS/Entity/MetadataProvider.h>
#include <algorithm>

class SparseComponentStorage {
protected:
    SecondaryArchetype& getOrCreateArchetype(const TypeUUID type) {
        if (auto* it = archetypes.find(type)) {
            return *it;
        }
        return archetypes.emplace(type, SecondaryArchetype::create(type, componentRegistry->getTypeInfoOf(type)));
    }

    SecondaryArchetype& getArchetypeUnchecked(const TypeUUID type) {
        return archetypes.getUnchecked(type);
    }

    SecondaryEntityLocation& findOrCreateLocation(const Entity& entity, const TypeUUID type) {
        auto& locs = metadata[entity].location;
        const auto it = std::ranges::lower_bound(locs, type, {}, &SecondaryEntityLocation::type);

        if (it != locs.end() && it->type == type) {
            return *it;
        }
        return locs.insert(it, type);
    }

    SecondaryEntityLocation* findLocation(const Entity& entity, const TypeUUID type) {
        auto& locs = metadata[entity].location;
        const auto it = std::ranges::lower_bound(locs, type, {}, &SecondaryEntityLocation::type);

        if (it != locs.end() && it->type == type) {
            return &*it;
        }
        return nullptr;
    }

    ComponentMap<SecondaryArchetype> archetypes;
    EntityMetadataStorage<SecondaryEntityMetadata>& metadata;
    ComponentKindRegistry<SecondaryField>* componentRegistry;
public:
    SparseComponentStorage(ComponentKindRegistry<SecondaryField>* componentRegistry, auto& metadata) : componentRegistry(componentRegistry), metadata(metadata) {}

    void confirmArchetypeExists(const TypeUUID type) {
        getOrCreateArchetype(type);
    }

    void add(const Entity* entities, TypeUUID type, void* data, size_t count);

    static void noFn(...) {}

    void remove(const Entity* entities, const TypeUUID type, size_t count) {
        auto& archetype = getOrCreateArchetype(type);

        for (size_t i = 0; i < count; ++i) {
            auto& entity = entities[i];

            if (auto* location = findLocation(entity, type)) [[likely]] {
                archetype.remove(metadata, *location);
                metadata[entity].erase(location);
            }
        }
    }

    void deleteEntity(const Entity& entity) {
        for (auto& location : metadata[entity].location) {
            auto& archetype = getArchetypeUnchecked(location.type);
            archetype.remove(metadata, location);
        }
        metadata[entity].clear();
    }

    void* get(const Entity& entity, const TypeUUID type) {
        if (auto* location = findLocation(entity, type)) {
            return location->data;
        }
        return nullptr;
    }

    template <typename T, typename Callable>
    void view(Callable&& callable) {
        static TypeUUID type = componentRegistry->getTypeID<T>();

        auto& archetype = getOrCreateArchetype(type);

        for (auto [entities, data, size] : archetype) {
            for (size_t i = 0; i < size; ++i) {
                callable(entities[i], *(reinterpret_cast<T*>(data) + i));
            }
        }
    }

    ComponentKindRegistry<SecondaryField>* getComponentRegistry() const {
        return componentRegistry;
    }

    void reset();
};
