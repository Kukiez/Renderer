#pragma once
#include "Component.h"
#include "ComponentKind.h"

class ComponentIndex {
    unsigned index = 0;
public:
    ComponentIndex() = default;

    explicit ComponentIndex(const unsigned index) : index(index) {}

    bool operator==(const ComponentIndex & other) const {
        return index == other.index;
    }

    bool operator!=(const ComponentIndex & other) const {
        return index != other.index;
    }

    bool operator > (const ComponentIndex & other) const {
        return index > other.index;
    }

    bool operator < (const ComponentIndex & other) const {
        return index < other.index;
    }

    unsigned id() const {
        return index;
    }

    template <typename T>
    static ComponentIndex of();
};

class ComponentTypeRegistry {
public:
    struct Impl;
private:
    static ComponentIndex getOrCreateComponentIndex(ComponentKind kind, const char* name);
    static ComponentKind getComponentKind(const char* name);
    static void initializeZeroComponentIndex(ComponentKind kind, const char* name);
public:
    template <typename T>
    requires IsComponent<T>
    static ComponentIndex getOrCreateComponentIndex(const ComponentKind kind) {
        static const ComponentIndex index = getOrCreateComponentIndex(kind, cexpr::name_of<std::decay_t<T>>);
        return index;
    }

    template <IsComponent T>
    static ComponentIndex getComponentIndex() {
        return getOrCreateComponentIndex<T>(ComponentTypeRegistry::getComponentKindOfType<typename T::ComponentType>());
    }

    template <typename T>
    static ComponentKind getComponentKindOfType() {
        static const ComponentKind kind = getComponentKind(cexpr::name_of<std::decay_t<T>>);
        return kind;
    }

    template <IsComponentType CType, typename ZeroType>
    static ComponentKind initializeZeroComponentIndex() {
        static std::monostate initializer = [&] {
            ComponentKind kind = getComponentKind(cexpr::name_of<CType>);
            initializeZeroComponentIndex(kind, cexpr::name_of<ZeroType>);
            return std::monostate{};
        }();

        return ComponentKind(0);
    }

    template <typename CType, typename T>
    static ComponentIndex getDirect() {
        return getOrCreateComponentIndex(getComponentKind(cexpr::name_of<CType>), cexpr::name_of<T>);
    }
};

template <typename CType, typename T>
struct ComponentIndexValue {
    static const inline auto value = ComponentTypeRegistry::getDirect<std::decay_t<CType>, std::decay_t<T>>();
};

template<typename T>
ComponentKind ComponentKind::of() {
    if constexpr (cexpr::is_base_of_template<ComponentType, T>) {
        return ComponentTypeRegistry::getComponentKindOfType<T>();
    } else if constexpr (IsComponent<T>) {
        using CType = ComponentTypeOf<T>;
        return ComponentTypeRegistry::getComponentKindOfType<CType>();
    } else {
        static_assert(false, "ComponentKind::of, Invalid TParam, Neither a ComponentType or Component");
    }
}

template<typename T>
bool ComponentKind::is() const {
    return myID == ComponentTypeRegistry::getComponentKindOfType<T>();
}

template <typename T>
ComponentIndex ComponentIndex::of() {
    return ComponentTypeRegistry::getOrCreateComponentIndex<T>();
}

std::ostream& operator << (std::ostream& os, ComponentKind kind);