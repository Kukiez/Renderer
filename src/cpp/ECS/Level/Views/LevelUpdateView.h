#pragma once
#include <ECS/Component/Types/Types.h>
#include "LevelAPI.h"
#include "BasicView.h"

template <typename Stage, typename System>
class LevelDeterministicView : public ViewBase {
    using Descriptor = SystemStageDescriptor<Stage, System>;
protected:
    Stage* stage;
public:
    using TSystem = System;
    using TStage = Stage;

    LevelDeterministicView(LevelContext& level, Stage* stage) : ViewBase(level), stage(stage) {}

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
        using QueryType = QueryTypeOf<typename Descriptor::template SelectConstness<Ts>...>;
        return QueryType(level());
    }

    template <typename T>
    requires (Descriptor::template CanReadComponent<T>() || Descriptor::template CanWriteComponent<T>())
    Descriptor::template SelectConstness<T>& get() const {
        return level().systemRegistry.getSystem<T>();
    }

    template <typename... Accesses>
    operator BasicView<Accesses...>() {
        ([] {
            using AccessedType = Descriptor::template SelectConstness<Accesses>;
            if constexpr (std::is_const_v<AccessedType> && !std::is_const_v<Accesses>) {
                static_assert(false, "LevelDeterministicView -> BasicView cast failed because a Component loses const qualifiers");
            }
        }(), ...);
        return BasicView<Accesses...>(level());
    }
};