#include "SystemScheduler.h"

#include "ECS/Level/LevelContext.h"
#include "Stages/UpdateStage.h"
#include "ECS/System/SystemCallContext.h"

Time SystemScheduler::runUpdateNode(ExecutionNode& node) {
    const auto now = Time::now();

    void* system = node.system.system;
    const auto update = node.system.update;
    update(system, level, inout);
    const auto end = Time::now();
    return end - now;
}

void SystemScheduler::enqueueUpdateNodeDeterministic(NodeResultsWriter& results, SystemExecutionGraph& graph, ExecutionNode& node) {
    threadPool.execute([&]{
        const auto result = runUpdateNode(node);
        results[static_cast<size_t>(node.systemLocalID)] += result;
    });
}

void SystemScheduler::enqueueUpdateNodeSerial(NodeResultsWriter& results, SystemExecutionGraph& graph, ExecutionNode& node) {

    results[static_cast<size_t>(node.systemLocalID)] += runUpdateNode(node);

}

void SystemScheduler::enqueueUpdateNodeParallel(NodeResultsWriter& results, SystemExecutionGraph& graph, ExecutionNode& node) {
    threadPool.execute([this, &node, &results] {
        results[static_cast<size_t>(node.systemLocalID)] += runUpdateNode(node);
    });
}

SystemScheduler::SystemScheduler(ThreadPool &tp, LevelContext &level): threadPool(tp), level(level) {
    inout = SystemCallContext(level.systemRegistry.getSystemKindRegistry());
}

void SystemScheduler::runUpdateStage(UpdateStage & stage, const SystemInvokeParams& invokeParams) {
    auto& executor = stage.getExecutionGraph();
    inout.setStage(stage.stageData);
    inout.setInvokeParams(invokeParams);

    auto writer = stage.getWriteSystemResults();
    switch (stage.stage->executionModel) {
        case StageExecutionModel::SERIAL:
            runSerialUpdateStage(writer, executor);
            break;
        case StageExecutionModel::PARALLEL:
            runParallelUpdateStage(writer, executor);
            break;
        case StageExecutionModel::DETERMINISTIC:
            runDeterministicUpdateStage(writer, executor);
            break;
        default:
            std::unreachable();
    }
}

void SystemScheduler::runDeterministicUpdateStage(NodeResultsWriter& results, SystemExecutionGraph& executor) {
    executor.runAtomic([&](ExecutionNode& node) {
        enqueueUpdateNodeDeterministic(results, executor, node);
    });
}

void SystemScheduler::runSerialUpdateStage(NodeResultsWriter& results, SystemExecutionGraph& executor) {
    for (auto& node : executor.nodes) {
        enqueueUpdateNodeSerial(results, executor, node);
    }
}

void SystemScheduler::runParallelUpdateStage(NodeResultsWriter& results, SystemExecutionGraph& executor) {
    for (auto& node : executor.nodes) {
        enqueueUpdateNodeParallel(results, executor, node);
    }
}