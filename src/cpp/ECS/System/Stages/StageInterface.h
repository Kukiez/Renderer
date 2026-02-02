#pragma once
#include <ECS/Component/Component.h>
#include <ECS/Component/ComponentKind.h>

#include "constexpr/Traits.h"

class SystemRegistry;

enum class StageExecutionModel {
    SERIAL,        /* Systems run one after another single-threaded, Dependencies can be specified */
    PARALLEL,      /* All Systems run in parallel */
    DETERMINISTIC, /* Systems run after another multithreaded, Conflicts and Dependencies can be specified */
    PASSIVE        /* Stage cannot be run */
};

enum class StageScheduleModel {
    PER_FRAME,
    FIXED_HZ,
    MANUAL,
    PASSIVE
};

struct StageComponent {
    using ComponentType = SystemRegistry;
};

template <typename>
struct Stage : StageComponent {
    template <typename... Ts>
    struct Dependencies {};

    template <typename... Ws>
    struct Writes {};

    template <typename... Rs>
    struct Reads {};

    template <typename... Ss>
    struct ReadsResources {};

    template <typename... Ss>
    struct WritesResources {};

    template <typename P>
    struct Pipeline {};
};

struct DefaultStage;

template <typename, typename> class LevelDeterministicView;

template <typename S>
using LevelUpdateView = LevelDeterministicView<DefaultStage, S>;

struct DefaultStage : Stage<DefaultStage> {
    using stage = DefaultStage;

    template <typename T>
    static constexpr auto Function = &T::onUpdate;

    template <typename T>
    constexpr static bool HasFunction = requires(T t) {
        &T::onUpdate;
    };

    static constexpr auto ExecutionModel = StageExecutionModel::DETERMINISTIC;
    static constexpr auto ScheduleModel = StageScheduleModel::PER_FRAME;

    template <typename System>
    using StageView = LevelUpdateView<System>;
};