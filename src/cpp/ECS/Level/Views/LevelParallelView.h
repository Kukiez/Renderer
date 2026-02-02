#pragma once
#include "LevelUpdateView.h"

template <typename Stage, typename System>
class LevelParallelView : public ViewBase {
    using Descriptor = SystemStageDescriptor<Stage, System>;
protected:
    Stage* stage;
public:
    using TSystem = System;
    using TStage = Stage;

    LevelParallelView(LevelContext& level, Stage* stage) : ViewBase(level), stage(stage) {}

    template <AreSameComponentType... Ts>
    auto get(const Entity& e) {
        return query<Ts...>().get(e);
    }

    template <AreSameComponentType... Ts>
    bool has(const Entity& e) {
        return query<Ts...>().has(e);
    }

    template <AreSameComponentType... Ts>
    auto query() const {
        using QueryType = QueryTypeOf<const Ts...>;
        return QueryType(level());
    }

    template <typename T>
    const auto& get() const {
        return level().systemRegistry.getSystem<T>();
    }

    template <typename... Accesses>
    operator BasicView<Accesses...>() {
        return BasicView<const Accesses...>(level());
    }
};