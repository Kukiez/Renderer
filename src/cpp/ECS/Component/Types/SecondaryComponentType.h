#pragma once
#include "AbstractComponentType.h"
#include "ECS/Entity/EntityRegistry.h"
#include "ECS/Entity/SparseComponentStorage.h"
#include "ECS/Forge/SecondaryStagingBuffer.h"

struct LevelContext;

class SecondaryComponentType : public ComponentType<SecondaryComponentType> {
    mem::free_list_allocator metadataAllocator;

    ComponentKindRegistry<SecondaryField> componentRegistry;
    EntityMetadataStorage<SecondaryEntityMetadata> metadata;
    SparseComponentStorage storage;
    SecondaryStagingBuffer staging;

public:
    SecondaryComponentType() : metadataAllocator(0.2 * 1024 * 1024), componentRegistry(Kind),
                                        metadata(500, &metadataAllocator),
                                        storage(&componentRegistry, metadata),
                                        staging(&componentRegistry) {
    }

    void onSynchronize(LevelContext& level);

    template <typename... Ts>
    void onCreateEntity(const Entity& e, Ts&&... components) {
        (staging.add(e, std::forward<Ts>(components)), ...);
    }

    template <typename... Ts>
    void onAddComponent(const Entity& e, Ts&&... components) {
        (staging.add(e, std::forward<Ts>(components)), ...);
    }

    template <typename... Ts>
    void onRemoveComponent(const Entity& e) {
        (staging.remove<Ts>(e), ...);
    }

    auto& getStorage() {
        return storage;
    }

    auto& getComponentRegistry() {
        return componentRegistry;
    }
};

namespace detail {
    class SecondaryQueryData {
    protected:
        SecondaryComponentType& storage;
    public:
        SecondaryQueryData(LevelContext& context);
    };
}


template <typename T>
class SecondaryEntityQuery : public EntityComponentQuery, public detail::SecondaryQueryData {
    template <typename Fn, typename Component>
    void invoke(Fn&& fn, const Entity& e, Component&& comp) {
        using FnArgs = cexpr::function_args_t<std::decay_t<Fn>>;
        using DecayedFnArgs = cexpr::decay_tuple_t<FnArgs>;
        using ComponentType = std::decay_t<Component>;

        constexpr bool hasComponent = cexpr::is_typename_in_tuple_v<ComponentType, DecayedFnArgs>;
        constexpr bool hasEntity = cexpr::is_typename_in_tuple_v<Entity, DecayedFnArgs>;

        constexpr size_t compIdx = cexpr::find_tuple_typename_index_v<ComponentType, DecayedFnArgs>;

        if constexpr (hasComponent && hasEntity) {
            if constexpr (compIdx == 0) {
                fn(comp, e);
            } else {
                fn(e, comp);
            }
        } else if constexpr (hasComponent) {
            fn(comp);
        } else if constexpr (hasEntity) {
            fn(e);
        } else {
            static_assert(false, "Function cannot be invoked with the given args");
        }
    }
public:
    explicit SecondaryEntityQuery(LevelContext& level) : SecondaryQueryData(level) {}

    T* get(const Entity e) {
        return static_cast<T *>(storage.getStorage().get(e, storage.getComponentRegistry().getTypeID<T>()));
    }

    bool has(const Entity e) {
        return get(e) != nullptr;
    }

    template <typename Fn>
    void forEach(Fn&& fn) {
        storage.getStorage().view<T>([&](const Entity& e, T& data) {
            invoke(fn, e, data);
        });
    }
};