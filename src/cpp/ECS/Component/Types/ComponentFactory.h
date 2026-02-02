#pragma once
#include "ECS/Entity/EntityRegistry.h"

class PrimaryComponentType;
class NameComponentType;
class BooleanComponentType;
class SecondaryComponentType;

class ComponentFactory : public EntityComponentFactory {
    EntityRegistry& registry;
    PrimaryComponentType& primary;
    SecondaryComponentType& secondary;
    BooleanComponentType& boolean;
    NameComponentType& nameComponent;

    template <typename... Ts>
    auto& selectStorage() const {
        using CType = ComponentTypeOf<Ts...>;
        if constexpr (std::is_same_v<CType, PrimaryComponentType>) {
            return primary;
        } else if constexpr (std::is_same_v<CType, SecondaryComponentType>) {
            return secondary;
        } else if constexpr (std::is_same_v<CType, BooleanComponentType>) {
            return boolean;
        } else if constexpr (std::is_same_v<CType, NameComponentType>) {
            return nameComponent;
        } else {
            static_assert(false, "?");
        }
    }
public:
    explicit ComponentFactory(LevelContext& level);

    template <AreSameComponentType... Cs>
    Entity createEntity(Cs&&... cs) {
        Entity e = registry.createEntity();

        auto& storage = selectStorage<Cs...>();
        storage.onCreateEntity(e, std::forward<Cs>(cs)...);
        return e;
    }

    template <typename... Cs>
    auto createEntityRet(Cs&&... cs) {
        Entity e = registry.createEntity();

        auto& storage = selectStorage<Cs...>();
        auto p = storage.onCreateEntity(e, std::forward<Cs>(cs)...);
        return std::make_pair(e, p);
    }

    template <AreSameComponentType... Cs>
    void add(const Entity e, Cs&&... cs) {
        auto& storage = selectStorage<Cs...>();
        storage.onAddComponent(e, std::forward<Cs>(cs)...);
    }

    template <AreSameComponentType... Cs>
    void remove(const Entity& e) const {
        auto& storage = selectStorage<Cs...>();
        storage.template onRemoveComponent<Cs...>(e);
    }

    void deleteEntity(const Entity& e) const {
        registry.deleteEntity(e);
    }
};
