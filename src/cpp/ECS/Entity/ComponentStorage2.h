#pragma once
#include <ECS/Entity/Archetype.h>
#include <ECS/Component/ComponentMap.h>
#include "ArchetypeUtils.h"
#include "Archetypeiterator.h"
#include "ECS/Forge/EntityCommandBuffer.h"
#include "ECS/Forge/PrimaryStagingBuffer.h"

class ComponentStorage2 {
    void initializeArchetype(size_t hash, Archetype& archetype, ArchetypeIndex index);

    TypeUUID findBestID(mem::range<TypeUUID> types);

    EntityMetadataStorage<EntityMetadata>& metadata;

    ComponentMap<mem::vector<ArchetypeIndex>> archIndices;
    std::unordered_map<size_t, ArchetypeIndex> archHashes;

    mem::vector<PrimaryArchetype> archetypes;

    PrimaryKindRegistry* componentRegistry;
public:
    ComponentStorage2(PrimaryKindRegistry* registry, EntityMetadataStorage<EntityMetadata>& metadata);

    void clearArchetypeTrackedChanges();

    Archetype& getArchetype(const Entity& entity);

    bool hasComponents(const Entity& entity) const {
        return metadata[entity].location.isValid();
    }

    void deleteEntity(const Entity& e);

    ArchetypeIndex findTargetArchetypeForEntity(EntityTypeIterator it, size_t addsHash);

    ArchetypeIndex findTargetArchetypeForEntity(ArchetypeIndex source, 
        mem::range<TypeUUID> removes, EntityTypeIterator adds, size_t removesHash, size_t addsHash);

    void createEntities(size_t hash, mem::range<TypeUUID> types, EntityCommandBuffer<CreateTag>* buffer);

    void processEntityBuffer(EntityDeferredOpsBuffer& buffer);

    template <typename T>
    T* get(const Entity& entity) {
        const auto& location = metadata[entity].location;

        if (location.archIndex == INVALID_INDEX<ArchetypeIndex>) return nullptr;
        auto& [archetype, edges] = archetypes[location.archIndex];

        return static_cast<T*>(archetype.getAt(location, componentRegistry->getTypeID<T>()));
    }

    template <typename... Ts>
    requires (sizeof...(Ts) > 1)
    auto get(const Entity& e) {
        return getAsTuple<Ts...>(e);
    }

    template <typename... Ts>
    auto getAsTuple(const Entity& e) {
        const auto& location = metadata[e].location;

        if (location.archIndex == INVALID_INDEX<ArchetypeIndex>) {
            return std::make_tuple(std::add_pointer_t<Ts>(0)...);
        }
        auto& [archetype, edges] = archetypes[location.archIndex];

        return std::make_tuple(
            static_cast<Ts*>(archetype.getAt(location, componentRegistry->getTypeID<Ts>()))...
        );
    }

    bool has(const Entity& entity, const TypeUUID type) {
        auto& location = metadata[entity].location;

        if (!location.isValid()) return false;

        auto& [archetype, edges] = archetypes[location.archIndex];
        return archetype.findType(type) != nullptr;
    }

    template <typename... Types>
    bool has(const Entity& entity) {
        auto& location = metadata[entity].location;

        if (!location.isValid()) return false;

        auto& [archetype, edges] = archetypes[location.archIndex];

        auto& types = componentRegistry->getSortedTypeRange<Types...>();

        size_t hint = 0;

        for (auto& type : types) {
            if (auto* typeIndex = archetype.findType(type, hint)) {
                hint = archetype.indexOf(typeIndex);
            } else {
                return false;
            }
        }
        return true;
    }

    auto& getMetadataOf(const Entity& e) const {
        return metadata[e];
    }

    template <typename... Ts>
    MatchingArchetypesIterator getMatchingArchetypes() {
        constexpr static auto MatchHash = cexpr::pack_stable_hash_v<Ts...>;

        auto bestType = findBestID(componentRegistry->getSortedTypeRange<Ts...>());

        auto& archVector = archIndices[bestType];

        return {
            componentRegistry,
            archVector.data(),
            archetypes.data(),
            archVector.size()
        };
    }

    auto& getArchetypes() {
        return archetypes;
    }

    void reset();
};
