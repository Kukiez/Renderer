#pragma once
#include <type_traits>

template <typename... Ts>
class PrimaryEntityQuery;

template <typename T>
class SecondaryEntityQuery;

template <typename T>
class BooleanQuery;

class EntityName;
class BooleanComponentType;
class SecondaryComponentType;
class PrimaryComponentType;

class ComponentFactory;

struct PrimaryComponent;
struct SecondaryComponent;
struct BooleanComponent;
struct TrackedComponent;

template <typename... Ts>
concept IsPrimaryComponent = (std::is_base_of_v<PrimaryComponent, std::decay_t<Ts>>  && ...);

template <typename... Ts>
concept IsSecondaryComponent = (std::is_base_of_v<SecondaryComponent, std::decay_t<Ts>> && ...);

template <typename... Ts>
concept IsBooleanComponent = (std::is_base_of_v<BooleanComponent, std::decay_t<Ts>> && ...);

template <typename ... Ts>
concept IsTrackedComponent = (std::is_base_of_v<TrackedComponent, std::decay_t<Ts>> && ...);

template <typename T>
concept IsNameComponent = std::is_same_v<std::decay_t<T>, EntityName>;

struct PrimaryComponent {
    using ComponentType = PrimaryComponentType;
    using FactoryType = ComponentFactory;

    template <typename... Ts>
    requires (IsPrimaryComponent<Ts...>)
    using QueryType = PrimaryEntityQuery<Ts...>;
};

struct SecondaryComponent {
    using ComponentType = SecondaryComponentType;
    using FactoryType = ComponentFactory;

    template <typename T>
    requires IsSecondaryComponent<T>
    using QueryType = SecondaryEntityQuery<T>;
};

struct BooleanComponent {
    using ComponentType = BooleanComponentType;
    using FactoryType = ComponentFactory;

    template <typename T>
    using QueryType = BooleanQuery<T>;
};

struct TrackedComponent : PrimaryComponent {};