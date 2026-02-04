#pragma once
#include "LevelContext.h"

struct LevelRuntime {
    std::atomic<bool> isFinished = false;
    bool firstRun = false;
    SteadyTime deltaTime = SteadyTime(0);

    ECSAPI void synchronize(Level& topLevel) const;

    ECSAPI void runUpdateStage(UpdateStage& stage, Level& topLevel, SystemInvokeParams invokeParams = {}) const;

    ECSAPI void runUpdateStages(auto& stages, Level& topLevel);

    ECSAPI void runLevel(Level& topLevel);

    ECSAPI void endFrame(Level& topLevel) const;
};
