#pragma once
#include "ECS/Component/ComponentRegistry.h"
#include "ECS/Entity/BooleanStorage.h"
#include "ECS/Entity/MetadataProvider.h"
#include "ECS/Forge/BooleanStagingBuffer.h"

struct LevelContext;

class BooleanComponentType : public ComponentType<BooleanComponentType> {
    mem::free_list_allocator metadataAllocator;
    ComponentKindRegistry<BooleanField> componentRegistry;
    EntityMetadataStorage<BooleanMetadata> metadata;
    BooleanStorage storage;
    BooleanStagingBuffer staging;
public:
    BooleanComponentType() :
        componentRegistry(Kind),
        metadataAllocator(0.1 * 1024 * 1024),
        metadata(500, &metadataAllocator), storage(&componentRegistry, metadata),
        staging(&componentRegistry)
    {}

    void onSynchronize(LevelContext& level);

    template <typename... Ts>
    void onCreateEntity(const Entity& e, Ts&&... components) {
        staging.add(e, std::forward<Ts>(components)...);
    }

    template <typename... Ts>
    void onAddComponent(const Entity& e, Ts&&... components) {
        staging.add(e, std::forward<Ts>(components)...);
    }

    template <typename... Ts>
    void onRemoveComponent(const Entity& e) {
        staging.remove<Ts...>(e);
    }

    auto& getStorage() { return storage; }
    auto& getComponentRegistry() { return componentRegistry; }
};

namespace detail {
    class BooleanQueryData {
    protected:
        BooleanComponentType& storage;
    public:
        BooleanQueryData(LevelContext& context);
    };
}

template <typename T>
class BooleanQuery : public EntityComponentQuery, public detail::BooleanQueryData {
public:
    explicit BooleanQuery(LevelContext& context) : BooleanQueryData(context) {}

    bool get(const Entity e) const {
        return storage.getStorage().has(e, storage.getComponentRegistry().getTypeID<T>());
    }

    bool has(const Entity e) const {
        return get(e);
    }
};