#pragma once
#include "LevelRuntime.h"
#include <ECS/System/Stages/SystemAssembler.h>
#include <ECS/Level/LevelView.h>

class Level : public LevelView<Level> {
protected:
    LevelContext ctx;
    LevelRuntime runtime;

    friend class LevelView;
public:
    Level(const Level&) = delete;
    Level& operator = (const Level&) = delete;

    Level(Level&&) = delete;
    Level& operator = (Level&&) = delete;

    ECSAPI explicit Level(const std::string &name);

    ECSAPI void initialize();

    template <typename S>
    requires IsSystem<S> || IsSystemPack<S>
    void addSystem() {
        if constexpr (!IsSystemPack<S>) {
            ctx.systemRegistry.addSystem<S>();
        }

        if constexpr (IsSystemPack<S>) {
            cexpr::for_each_typename_in_tuple<typename S::Pack>([&]<typename... Ts>() {
                (addSystem<Ts>(), ...);
            });
        }
    }

    template <IsComponentType T, typename... Args>
    void addComponentType(Args&&... args) {
        if (ctx.registry.hasComponentType(T::Kind)) {
            return;
        }
        AbstractComponentType type = AbstractComponentType::of<T>();

        type.instance = new T(std::forward<Args>(args)...);
        ctx.registry.addComponentType(type);
    }

    template <IsComponentType T>
    T& getComponentType() {
        return ctx.registry.getComponentType<T>();
    }

    template <typename... Detectors, typename Derived>
    SystemAssembler<Derived, Detectors...> createSystemAssembler(this Derived& derived) {
        return SystemAssembler<Derived, Detectors...>(derived);
    }

    template <typename System>
    void onAddSystem() {

    }


    template <IsStage Stage>
    Stage& addStage(auto&&... args) {
        return ctx.systemRegistry.addStage<Stage>(std::forward<decltype(args)>(args)...);
    }

    template <IsActiveStage Stage, typename Enumerable, typename... Args>
    requires std::constructible_from<Enumerable, Args...>
    bool addSystemEnumerable(Args&&... args) {
        return ctx.systemRegistry.addSystemEnumerable<Stage, Enumerable>(std::forward<Args>(args)...);
    }

    template <typename Stage>
    Stage& getStage() {
        return ctx.systemRegistry.getStage<Stage>();
    }

    template <typename Sys>
    Sys& getSystem() {
        return ctx.systemRegistry.getSystem<Sys>();
    }

    ECSAPI void run();
    ECSAPI void endFrame();

    template <IsActiveStage Stage>
    void run() {
        auto& stage = ctx.systemRegistry.getUpdateStage<Stage>();
        runtime.runUpdateStage(stage, *this);
    }

    LevelContext& internal() {
        return ctx;
    }

    void synchronize() {
        runtime.synchronize(*this);
    }

    template <AreSameComponentType... Ts>
    auto get(const Entity& e) {
        return query<Ts...>().get(e);
    }

    template <AreSameComponentType... Ts>
    bool has(const Entity& e) {
        return query<Ts...>().has(e);
    }

    template <AreSameComponentType... Ts>
    auto query() {
        using QueryType = QueryTypeOf<Ts...>;
        return QueryType(level());
    }

    LevelContext& level() {
        return ctx;
    }

    operator LevelContext& () {
        return ctx;
    }

    EntityID getEntityLimit() const {
        return ctx.registry.getEntityLimit();
    }

    auto& getRuntime() {
        return runtime;
    }
};
