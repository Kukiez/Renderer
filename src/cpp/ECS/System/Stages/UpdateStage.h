#pragma once
#include <unordered_map>

#include "StageInterface.h"
#include "SystemGraph.h"
#include "ECS/Component/TypeUUID.h"
#include "ECS/System/RuntimeSystemDescriptor.h"
#include "ECS/System/StageKindRegistry.h"
#include "ECS/System/SystemVirtualTable.h"
#include "memory/byte_arena.h"

class SystemKindRegistry;
struct ExecutionGraphCreationContext;

template <> struct std::hash<std::pair<int, int>> {
    size_t operator()(const std::pair<int, int>& p) const noexcept {
        const size_t h1 = std::hash<int>{}(p.first);
        const size_t h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

struct ExecutionNode {
    struct DependenciesRemaining {
        std::atomic<int> atomic = 0;
        int value = 0;

        DependenciesRemaining() = default;
        DependenciesRemaining(const DependenciesRemaining& other)
            : atomic(other.atomic.load()), value(other.value) {}

        DependenciesRemaining(DependenciesRemaining&& other) noexcept
            : atomic(other.atomic.load()), value(other.value) {}

        DependenciesRemaining& operator=(const DependenciesRemaining& other) {
            if (this != &other) {
                atomic.store(other.atomic.load());
                value = other.value;
            }
            return *this;
        }

        DependenciesRemaining& operator=(DependenciesRemaining&& other) noexcept {
            if (this != &other) {
                atomic.store(other.atomic.load());
                value = other.value;
            }
            return *this;
        }
    };

    TypeUUID systemID;
    StageLocalIndex systemLocalID = {};
    UpdateSystemFnTable system;
    int* outEdgesPtr = nullptr;
    int outEdgesCount = 0;
    DependenciesRemaining dependenciesRemaining;
    int dependencies = 0;

    ExecutionNode() = default;
    ExecutionNode(const UpdateSystemFnTable table, const TypeUUID system, const StageLocalIndex localID) : systemID(system), system(table), systemLocalID(localID) {}

    ExecutionNode(ExecutionNode&& other) noexcept
        : systemID(other.systemID), systemLocalID(other.systemLocalID),
            system(other.system), outEdgesPtr(other.outEdgesPtr),
            outEdgesCount(other.outEdgesCount),
            dependenciesRemaining(other.dependenciesRemaining),
            dependencies(other.dependencies) {}

    ExecutionNode& operator=(ExecutionNode&& other) noexcept {
        if (this != &other) {
            systemID = other.systemID;
            system = other.system;
            outEdgesPtr = other.outEdgesPtr;
            outEdgesCount = other.outEdgesCount;
            dependenciesRemaining = other.dependenciesRemaining;
            dependencies = other.dependencies;
            systemLocalID = other.systemLocalID;
        }
        return *this;
    }

    auto outEdges() const {
        return mem::make_range(outEdgesPtr, outEdgesCount);
    }
};

struct alignas(32) NodeResult {
    Time fastestExecution = Time(0);
    Time averageExecution = Time(0);
    Time slowestExecution = Time(0);
    size_t totalExecutions = 0;

    NodeResult() = default;

    void addResult(const Time time) {
        if (totalExecutions == 0) {
            fastestExecution = time;
            slowestExecution = time;
        }
        fastestExecution = std::min(fastestExecution, time);
        averageExecution += time;
        slowestExecution = std::max(slowestExecution, time);
        ++totalExecutions;
    }

    NodeResult& operator += (const Time time) {
        addResult(time);
        return *this;
    }
};

class SystemExecutionGraph {
    template <typename Fn, typename Proj>
    void runImpl(ExecutionNode& node, Fn&& fn, Proj proj) {
        fn(node);

        for (const int edges : node.outEdges()) {
            if (--(nodes[edges].dependenciesRemaining.*proj) == 0) {
                runImpl(nodes[edges], fn, proj);
            }
        }
    }
public:
    std::unique_ptr<int[]> ins;
    mem::vector<ExecutionNode> nodes;

    ExecutionNode& operator [] (const auto idx) {
        return nodes[idx];
    }

    void clear() {
        nodes.clear();
        ins.reset();
    }

    template <typename Fn>
    void runAtomic(Fn&& fn) {
        for (auto& node : nodes) {
            node.dependenciesRemaining.atomic = node.dependencies;
        }

        for (auto& node : nodes) {
            if (node.dependencies != 0) continue;
            runImpl(node, fn, &ExecutionNode::DependenciesRemaining::atomic);
        }
    }

    template <typename Fn>
    void run(Fn&& fn) {
        for (auto& node : nodes) {
            node.dependenciesRemaining.value = node.dependencies;
        }
        for (auto& node : nodes) {
            if (node.dependencies != 0) continue;
            runImpl(node, fn, &ExecutionNode::DependenciesRemaining::value);
        }
    }

    template <typename Fn>
    void runSerial(Fn&& fn) {
        for (auto& node : nodes) {
            fn(node);
        }
    }
};

struct Batch {
    mem::vector<int> systems;

    bool conflictsWith(SystemGraph& graph, const int system) {
        for (const int sys : systems) {
            if (graph[sys].conflicts(graph[system])) {
                return true;
            }
        }
        return false;
    }

    auto& add(const int sys) {
        return systems.emplace_back(sys);
    }
};

struct StageProcessor {
    ExecutionGraphCreationContext& ctx;
    SystemGraph& graph;
    SystemExecutionGraph& executor;

    mem::vector<mem::vector<int, mem::byte_arena_adaptor<int>>, mem::byte_arena_adaptor<mem::vector<int, mem::byte_arena_adaptor<int>>>> tempExecutionGraphIndices;
    int totalGraphIndices = 0;

    std::unordered_map<int, mem::vector<int>> layers;
    mem::vector<Batch, mem::byte_arena_adaptor<Batch>> batches;

    union {
        int* systemIdxToExecutionNodeIdx = nullptr;
        int* systemIdxToInDegrees;
    }; // Allocated to graph.size(); and reused by runTopologicalSort() & addExecutionNode

    StageProcessor(ExecutionGraphCreationContext& ctx, SystemGraph& graph,
        SystemExecutionGraph& executor, StageExecutionModel model);

    void runTopologicalSort();

    void linkSystems(int parentExeNode, int childExeNode);

    void tryLinkSystems(int currentGraphNode, int ancestorGraphNode);

    void emplaceExecutionNode(SystemGraph::Node& node);

    void addExecutionNode(Batch* batch, int systemIdx);

    void createExecutionGraph();

    void finalizeExecutionGraph();

    void createExecutionGraphIgnoreDependencies();
};

struct SystemExecutionResults {
    mem::vector<NodeResult> results;

    auto& operator [] (const size_t idx) {
        return results[idx];
    }

    void assign(const size_t size, const NodeResult& result) {
        results.assign(size, result);
    }
};

struct NodeResultsWriter {
    SystemExecutionResults& results;
    std::atomic<bool>& writing;

    NodeResultsWriter(SystemExecutionResults& results, std::atomic<bool>& writing)
        : results(results), writing(writing) {
        writing = true;
    }

    auto& operator [] (const size_t idx) const {
        return results[idx];
    }

    NodeResultsWriter(const NodeResultsWriter&) = delete;
    NodeResultsWriter& operator = (const NodeResultsWriter&) = delete;
    NodeResultsWriter(NodeResultsWriter&&) = delete;
    NodeResultsWriter& operator = (NodeResultsWriter&&) = delete;

    ~NodeResultsWriter() {
        writing = false;
    }
};

struct NodeResultsReader {
    SystemExecutionResults& results;
    std::atomic<uint8_t>& readers;

    NodeResultsReader(SystemExecutionResults& results, std::atomic<uint8_t>& readers)
        : results(results), readers(readers) {
        ++readers;
    }

    auto& operator [] (const size_t idx) const {
        return results[idx];
    }

    NodeResultsReader(const NodeResultsReader&) = delete;
    NodeResultsReader& operator = (const NodeResultsReader&) = delete;
    NodeResultsReader(NodeResultsReader&&) = delete;
    NodeResultsReader& operator = (NodeResultsReader&&) = delete;

    ~NodeResultsReader() {
        --readers;
    }
};

struct UpdateStageMetrics {
    size_t totalExecutions = 0;
    Time totalExecutionTime = Time(0);
    Time fastestExecutionTime = Time(0);
    Time slowestExecutionTime = Time(0);

    Time onStageBeginCounter = Time(0);
    Time onStageEndCounter = Time(0);
};

struct UpdateStage {
    SystemGraph graph;
    SystemExecutionGraph executor;
    SteadyTime timeAccumulator = SteadyTime(0);
    size_t version = 0;
    void* stageData{};
    const RuntimeStageDescriptor* stage{};
    UpdateStageMetrics metrics;

    SystemExecutionResults results;
    std::atomic<bool> writing = false;
    std::atomic<uint8_t> readers;

    size_t size() const {
        return graph.size();
    }

    void onStageBegin(Level& level);

    void onStageEnd(Level& level);

    template <typename Fn>
    void forEachSystem(Fn&& fn) {
        switch (stage->executionModel) {
            case StageExecutionModel::DETERMINISTIC: {
                executor.run(std::forward<Fn>(fn));
            }
            case StageExecutionModel::PARALLEL:
            case StageExecutionModel::SERIAL:
                for (auto& node : executor.nodes) {
                    fn(node.systemID);
                }
                break;
            default:
                std::unreachable();
        }
    }

    template <typename Stage>
    auto& getStage() {
        return *static_cast<Stage*>(stageData);
    }

    UpdateStage() = default;

    explicit UpdateStage(const RuntimeStageDescriptor& descriptor, void* stageData)
        : stageData(stageData), stage(&descriptor)
    {}

    bool isOutdated() const {
        return version != 0;
    }

    bool isFixedDelta() const {
        return stage->hz != SteadyTime(0);
    }

    void step(const SteadyTime time) {
        timeAccumulator += time;
    }

    void tick() {
        timeAccumulator -= stage->hz;
    }

    bool tickable() const {
        return timeAccumulator >= stage->hz;
    }

    void addSystem(RuntimeSystemStageDescriptor &descriptor, TypeUUID type, void* instance);

    auto& getExecutionGraph() {
        return executor;
    }

    void createExecutionGraph(ExecutionGraphCreationContext& ctx);

    bool isRunnable() const {
        return !executor.nodes.empty();
    }

    void dumpMermaid(const SystemKindRegistry& registry);

    void discard(const size_t graphIndex);

    auto& systems() {
        return graph.nodes;
    }

    auto& systems() const {
        return graph.nodes;
    }

    auto getReadSystemResults() {
        while (writing) {}
        return NodeResultsReader(results, readers);
    }

    auto getWriteSystemResults() {
        while (readers) {}
        return NodeResultsWriter(results, writing);
    }
};