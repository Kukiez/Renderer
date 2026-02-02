#pragma once

#include <tbb/tbb.h>
#include "MetadataProvider.h"
#include "ECS/Component/ComponentRegistry.h"
#include "ECS/Component/Types/AbstractComponentType.h"
#include "ECS/Forge/DeletedEntitiesBuffer.h"

class EntityReference;

struct EntityCreator {
    EntityID next{};
    EntityID cap{};
    std::vector<Entity> ids;

    bool hasEntity() const {
        return next != cap || !ids.empty();
    }

    void setSlice(const EntityID first, const EntityID last) {
        next = first;
        cap = last;
    }

    Entity getEntity() {
        if (!ids.empty()) {
            const auto curr = ids.back();
            ids.pop_back();
            return curr;
        }
        return Entity(0, next++);
    }

    void recycleEntity(const Entity& e) {
        ids.emplace_back(e);
    }
};

struct AbstractComponentsIteratorSentinelEnd {};

struct AbstractComponentsIterator {
    AbstractComponentType* first;
    AbstractComponentType* last;

    AbstractComponentsIterator(AbstractComponentType* first, AbstractComponentType* last) : first(first), last(last) {}

    bool operator == (const AbstractComponentsIteratorSentinelEnd&) const {
        return first == last;
    }

    bool operator != (const AbstractComponentsIteratorSentinelEnd&) const {
        return first != last;
    }

    AbstractComponentType& operator* () const {
        return *first;
    }

    void increment() {
        ++first;

        while (first != last && !first->instance) {
            ++first;
        }
    }

    AbstractComponentsIterator& operator++ () {
        increment();
        return *this;
    }

    AbstractComponentsIterator operator ++ (int) {
        auto copy = *this;
        increment();
        return copy;
    }
};

struct AbstractComponentsIteratable {
    AbstractComponentType* first;
    AbstractComponentType* last;

    auto begin() {
        return AbstractComponentsIterator(first, last);
    }

    auto end() {
        return AbstractComponentsIteratorSentinelEnd{};
    }
};

class EntityRegistry {
    struct EntityInfo {
        uint8_t generation = 0;
    };
    EntityID defaultEntityCapacity = 5'000;
    EntityID threadLocalEntityCapacity = 5'000 / std::thread::hardware_concurrency() / 4;

    std::atomic<EntityID> nextEntityID = 1;

    tbb::enumerable_thread_specific<EntityCreator> entityCreator;

    EntityMetadataStorage<EntityInfo> metadata;
    std::vector<AbstractComponentType> components;

    DeletedEntitiesBuffer deletedEntitiesQueue;
    std::vector<Entity> deletedEntities;
public:
    EntityRegistry(EntityID initialCapacity, EntityID threadLocalCapacity);

    EntityRegistry(const EntityRegistry&) = delete;
    EntityRegistry& operator = (const EntityRegistry&) = delete;

    EntityRegistry(EntityRegistry&&) = delete;
    EntityRegistry& operator = (EntityRegistry&&) = delete;

    ~EntityRegistry() = default;

     EntityID incrementEntityGeneration(Entity entity);

    void deleteEntity(const Entity e);

    Entity create();

    void addComponentType(const AbstractComponentType &type);

    bool hasComponentType(const ComponentKind kind) const;

    AbstractComponentType& getComponentType(const ComponentKind kind) {
        return components[static_cast<int>(kind)];
    }

    template <typename ComponentType>
    ComponentType& getComponentType() {
        const ComponentKind kind = ComponentTypeRegistry::getComponentKindOfType<ComponentType>();
        return *static_cast<ComponentType*>(getComponentType(kind).instance);
    }

    Entity createEntity() {
        return create();
    }

    uint8_t getLiveGeneration(const Entity& e) {
        if (e.id() > metadata.getCapacity()) return 255;
        return metadata[e].generation;
    }

    EntityID getEntityLimit() const {
        return static_cast<EntityID>(metadata.getCapacity());
    }

    void setEntityLimit(const EntityID limit) {
        defaultEntityCapacity = limit;
    }

    void reset();

    AbstractComponentsIteratable getComponents() {
        return {components.data(), components.data() + components.size()};
    }

    void onSynchronize(LevelContext& level);

    DeletedEntities getDeletedEntities() const {
        return DeletedEntities(deletedEntities);
    }
};