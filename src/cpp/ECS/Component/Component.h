#pragma once
#include <memory/type_info.h>
#include <variant>

#include "ComponentKind.h"
#include "constexpr/Template.h"

template <typename T>
struct ComponentType {
    static inline ComponentKind Kind = ComponentKind::of<T>();
};

struct EntityComponentFactory {};

struct EntityComponentQuery {};

template <typename Field, typename Component, typename... Args>
concept FieldConstructibleFrom = requires(Args... args)
{
    Field::template of<Component>(args...);
};

template <typename Field>
struct ComponentField : Field {
    const mem::type_info* type{};

    ComponentField() : type(mem::type_info_of<void>) {}

    template <typename T, typename... Args>
    ComponentField(std::type_identity<T>, Args&&... args) : Field(Field::template of<T>(std::forward<Args>(args)...)), type(mem::type_info_of<T>) {}

    template <typename T, typename... Args>
    requires FieldConstructibleFrom<Field, T, Args...>
    static ComponentField of(Args&&... args) {
        ComponentField field(std::type_identity<T>{}, std::forward<Args>(args)...);
        return field;
    }

    size_t hash() const {
        return type->hash;
    }
};

struct SecondaryField {
    template <typename T>
    static SecondaryField of() {
        return {};
    }
};

struct BooleanField {
    bool isLookupOnly = true;

    template <typename T>
    static BooleanField of() {
        return {};
    }
};

template <typename T>
concept IsComponent = requires
{
    typename T::ComponentType;
};

template <typename T>
concept IsComponentType = cexpr::is_base_of_template<ComponentType, T>;

template <typename T>
concept IsFactoryType = std::is_base_of_v<EntityComponentFactory, T>;

template <typename T>
concept IsQueryType = std::is_base_of_v<EntityComponentQuery, T>;

template <typename... Ts>
concept AreSameComponentType = requires {
    [] {
        using First = std::tuple_element_t<0, std::tuple<std::decay_t<Ts>...>>::ComponentType;

        return (std::is_same_v<First, typename std::decay_t<Ts>::ComponentType> && ...);
    }();
};

template <typename... Ts>
using ComponentTypeOf = std::tuple_element_t<0, std::tuple<std::decay_t<Ts>...>>::ComponentType;

template <typename... Ts>
using QueryTypeOf = std::tuple_element_t<0, std::tuple<std::decay_t<Ts>...>>::template QueryType<Ts...>;

template <typename... Ts>
using FactoryTypeOf = std::tuple_element_t<0, std::tuple<std::decay_t<Ts>...>>::FactoryType;