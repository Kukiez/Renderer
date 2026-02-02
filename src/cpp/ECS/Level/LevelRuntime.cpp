#pragma once
#include "LevelRuntime.h"
#include <ECS/System/SystemScheduler.h>
#include <ECS/Forge/EntityCommandBuffer.h>

#include "Level.h"

void LevelRuntime::runUpdateStage(UpdateStage& stage, Level& topLevel, SystemInvokeParams invokeParams) const {
    SystemScheduler scheduler(topLevel.internal().threadPool, topLevel);
    stage.onStageBegin(topLevel);

    if (stage.isRunnable()) {
        if (stage.isFixedDelta()) {
            stage.step(deltaTime);

            while (stage.tickable()) {
                stage.tick();
                scheduler.runUpdateStage(stage, invokeParams);
            }
        } else {
            const Time now = Time::now();
            scheduler.runUpdateStage(stage, invokeParams);
            const Time elapsed = now - Time::now();
            ++stage.metrics.totalExecutions;
            stage.metrics.fastestExecutionTime = std::min(stage.metrics.fastestExecutionTime, elapsed);
            stage.metrics.slowestExecutionTime = std::max(stage.metrics.slowestExecutionTime, elapsed);
            stage.metrics.totalExecutionTime += elapsed;
        }
    }
    stage.onStageEnd(topLevel);
}

void LevelRuntime::runUpdateStages(auto& stages, Level& topLevel) {
    for (UpdateStage* stage : stages) {
        runUpdateStage(*stage, topLevel);
    }
}

void LevelRuntime::synchronize(Level& topLevel) const {
    topLevel.internal().registry.onSynchronize(topLevel.internal());

    for (const auto& registry : topLevel.internal().registry.getComponents()) {
        if (registry.onSynchronize) {
            registry.onSynchronize(registry.instance, topLevel.internal());
        }
    }
}

void LevelRuntime::runLevel(Level& topLevel) {
    const SteadyTime now = SteadyTime::now();
    deltaTime = now - topLevel.internal().lastFrame;
    if (deltaTime.seconds() > 0.25) deltaTime = Time(0.25);

    topLevel.internal().lastFrame = now;

    runUpdateStages(topLevel.internal().systemRegistry.getPerTickStages(), topLevel);

    synchronize(topLevel);

    endFrame(topLevel);
}

void LevelRuntime::endFrame(Level& topLevel) const {
    topLevel.internal().asyncOps.poll();
    topLevel.internal().frameAllocator.reset();

    for (const auto& registry : topLevel.internal().registry.getComponents()) {
        if (registry.onFrameEnd) {
            registry.onFrameEnd(registry.instance, topLevel.internal());
        }
    }
}
