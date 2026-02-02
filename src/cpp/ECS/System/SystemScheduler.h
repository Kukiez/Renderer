#pragma once

#include "SystemCallContext.h"
#include "ECS/Time.h"

class SystemExecutionGraph;
struct UpdateStage;
struct LevelContext;
struct ExecutionNode;
struct NodeResultsWriter;
class SystemScheduler;
class ThreadPool;
struct LevelContextRef;

class SystemScheduler {
    Time runUpdateNode(ExecutionNode& node);

    void enqueueUpdateNodeDeterministic(NodeResultsWriter& results, SystemExecutionGraph& graph, ExecutionNode& node);

    void enqueueUpdateNodeSerial(NodeResultsWriter& results, SystemExecutionGraph& graph, ExecutionNode& node);

    void enqueueUpdateNodeParallel(NodeResultsWriter& results, SystemExecutionGraph& graph, ExecutionNode& node);

    ThreadPool& threadPool;
    LevelContext& level;
    SystemCallContext inout;
public:
    SystemScheduler(ThreadPool& tp, LevelContext& level);

    void runUpdateStage(UpdateStage & stage, const SystemInvokeParams& invokeParams = {});

    void runDeterministicUpdateStage(NodeResultsWriter& results, SystemExecutionGraph& executor);
    
    void runSerialUpdateStage(NodeResultsWriter& results, SystemExecutionGraph& executor);

    void runParallelUpdateStage(NodeResultsWriter& results, SystemExecutionGraph& executor);
};