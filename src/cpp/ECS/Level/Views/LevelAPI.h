#pragma once
#include <unordered_set>
#include <ECS/System/SystemProfileReport.h>

#include "ECS/Level/LevelContext.h"

template <typename Derived>
class LevelView {
public:
    // Utility Functions

    template <IsFactoryType Factory>
    Factory getFactory(this Derived& derived) {
        return Factory(derived.level());
    }

    template <IsComponent Component>
    auto getFactory(this Derived& derived) {
        using Fac = FactoryTypeOf<Component>;

        return derived.template getFactory<Fac>();
    }

    template <AreSameComponentType... Ts>
    auto createEntity(this Derived& derived, Ts&&... components) {
        return derived.template getFactory<FactoryTypeOf<Ts...>>().createEntity(std::forward<Ts>(components)...);
    }

    template <typename... Ts>
    auto add(this Derived& derived, const Entity& e, Ts&&... components) {
        return derived.template getFactory<FactoryTypeOf<Ts...>>().add(e, std::forward<Ts>(components)...);
    }

    template <typename... Ts>
    auto remove(this Derived& derived, const Entity e) {
        return derived.template getFactory<FactoryTypeOf<Ts...>>().template remove<Ts...>(e);
    }

    // Allocate Memory

    template <
        template <typename...> typename Container, typename Key,
        typename Value, typename Hash = std::hash<Key>,
        typename Eq = std::equal_to<Key>
    >
    requires (cexpr::is_same_template_v<std::unordered_map, Container>)
    auto allocate(this Derived& derived, const size_t capacity = 0) {
        return derived.level().thisFrame.allocateMap<Key, Value, Hash, Eq>(capacity);
    }

    template <template <typename...> typename Container, typename T>
    requires (cexpr::is_same_template_v<std::vector, Container>)
    auto allocate(this Derived& derived, const size_t capacity = 0) {
        return derived.level().thisFrame.allocateVector<T>(capacity);
    }

    template <template <typename...> typename Container, typename T, typename Hash = std::hash<T>, typename Eq = std::equal_to<T>>
    requires (cexpr::is_same_template_v<std::unordered_set, Container>)
    auto allocate(this Derived& derived, const size_t capacity = 0) {
        return derived.level().thisFrame.allocateHashSet<T, Hash, Eq>(capacity);
    }

    template <typename T>
    requires std::is_same_v<std::string, T>
    auto allocate(this Derived& derived, const size_t capacity = 0) {
        return derived.level().thisFrame.allocateString(capacity);
    }

    template <typename T>
    requires std::is_same_v<std::string, T>
    auto allocate(this Derived& derived, const std::string_view chars) {
        return derived.level().thisFrame.allocateString(chars);
    }

    void* allocate(this Derived& derived, const size_t capacity) {
        return derived.level().thisFrame.allocateRaw(capacity);
    }

    template <typename T, typename... Args>
    T& allocate(this Derived& derived, Args&&... args) {
        return *derived.level().thisFrame.template allocateType<T>(std::forward<Args>(args)...);
    }

    FrameAllocator& getFrameAllocator(this Derived& derived) {
        return derived.level().frameAllocator;
    }
};

class ViewBase : public LevelView<ViewBase> {
protected:
    LevelContext* ctx = nullptr;

    LevelContext& level() const {
        return *ctx;
    }

    operator LevelContext& () const {
        return *ctx;
    }

    friend class LevelView;
public:
    ViewBase(LevelContext& level) : ctx(&level) {}
};