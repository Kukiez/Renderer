#pragma once
#include "LevelContext.h"

struct LevelRuntime {
    std::atomic<bool> isFinished = false;
    bool firstRun = false;
    SteadyTime deltaTime = SteadyTime(0);

    void synchronize(Level& topLevel) const;

    void runUpdateStage(UpdateStage& stage, Level& topLevel, SystemInvokeParams invokeParams = {}) const;

    void runUpdateStages(auto& stages, Level& topLevel);

    void runLevel(Level& topLevel);

    void endFrame(Level& topLevel) const;
};
