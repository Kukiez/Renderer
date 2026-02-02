#pragma once
#include "AbstractComponentType.h"
#include "PrimaryKindRegistry.h"
#include "ECS/Forge/PrimaryStagingBuffer.h"
#include "ECS/Entity/ComponentStorage2.h"
#include "ECS/Entity/EntityRegistry.h"

class PrimaryComponentType : public ComponentType<PrimaryComponentType> {
    PrimaryKindRegistry componentRegistry;
    EntityMetadataStorage<EntityMetadata> metadata;
    ComponentStorage2 storage;
    PrimaryStagingBuffer staging;
    EntityDeferredOpsBuffer stagingBuffer{};

    mem::vector<AppendCommandBuffer> entityAppends{};
    mem::vector<RemoveCommandBuffer> entityRemovals{};
    mem::vector<TypeUUID> removes{};
public:
    explicit PrimaryComponentType()
    : componentRegistry(Kind), storage(&componentRegistry, metadata), staging(&componentRegistry)  {}

    void onSynchronize(LevelContext& level);

    void onFrameEnd(LevelContext& level);

    template <typename... Ts>
    auto onCreateEntity(const Entity& e, Ts&&... components) {
        return staging.create(e, std::forward<Ts>(components)...);
    }

    template <typename... Ts>
    void onAddComponent(const Entity& e, Ts&&... components) {
        staging.add(e, std::forward<Ts>(components)...);
    }

    template <typename... Ts>
    void onRemoveComponent(const Entity& e) {
        staging.remove<Ts...>(e);
    }

    PrimaryKindRegistry& getComponentRegistry() {
        return componentRegistry;
    }

    ComponentStorage2& getStorage() {
        return storage;
    }

    void readyChanges();

    void commitEntityCreations();

    void dumpArchetypes();
};

struct PrimaryEntityQueryData {
protected:
    PrimaryComponentType& storage;
public:
    explicit PrimaryEntityQueryData(LevelContext& level);
};

template <typename... Ts>
class PrimaryEntityQuery : public EntityComponentQuery, public PrimaryEntityQueryData {
public:
    explicit PrimaryEntityQuery(LevelContext& level) : PrimaryEntityQueryData(level) {}

    template <typename T>
    requires cexpr::is_typename_in_tuple_v<T, std::tuple<Ts...>>
    T* get(const Entity& e) requires (sizeof...(Ts) > 1) {
        return storage.getStorage().get<T>(e);
    }

    auto get(const Entity& e) const requires (sizeof...(Ts) == 1) {
        return storage.getStorage().get<Ts...>(e);
    }

    std::tuple<Ts*...> get(const Entity& e) requires (sizeof...(Ts) > 1) {
        return storage.getStorage().get<Ts...>(e);
    }

    template <typename T>
    bool has(const Entity& e) const {
        return storage.getStorage().has<T>(e);
    }

    bool has(const Entity& e) const {
        return storage.getStorage().has<Ts...>(e);
    }

    template <typename Fn>
    auto& forEach(Fn&& fn) {
        auto matching = storage.getStorage().getMatchingArchetypes<Ts...>();
        matching.forEach([&](const Entity& e, Ts&... primaries) {
            using FnArgs = cexpr::function_args_t<std::decay_t<Fn>>;

            using FirstArg = std::decay_t<std::tuple_element_t<0, FnArgs>>;

            if constexpr (std::is_same_v<FirstArg, Entity>) {
                fn(e, primaries...);
            } else {
                fn(primaries...);
            }
        });
        return *this;
    }

    template <IsPrimaryComponent Changed, typename Fn>
    requires (IsTrackedComponent<Changed>)
    void forEachChanged(Fn&& fn) {
        auto matching = storage.getStorage().getMatchingArchetypes<Ts...>();

        matching.template forEachChanged<Changed>([&](const Entity& e, Ts&... primaries) {
            using FnArgs = cexpr::function_args_t<std::decay_t<Fn>>;

            if constexpr (std::is_same_v<std::decay_t<std::tuple_element_t<0, FnArgs>>, Entity>) {
                fn(e, primaries...);
            } else {
                fn(primaries...);
            }
        });
    }
};