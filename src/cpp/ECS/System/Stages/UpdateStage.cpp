#include "UpdateStage.h"
#include <queue>
#include <ranges>
#include <unordered_map>

#include "StageInterface.h"
#include "ECS/System/RuntimeSystemDescriptor.h"
#include "ECS/System/ExecutionGraphCreationContext.h"
#include "ECS/System/SystemKindRegistry.h"

StageProcessor::StageProcessor(ExecutionGraphCreationContext& ctx, SystemGraph& graph,
                               SystemExecutionGraph& executor, StageExecutionModel model)
    : ctx(ctx), graph(graph), executor(executor), batches(&ctx.allocator), tempExecutionGraphIndices(&ctx.allocator)
{
    executor.nodes.reserve(graph.size());
    tempExecutionGraphIndices.reserve(graph.size());
    systemIdxToExecutionNodeIdx = static_cast<int*>(ctx.allocator.allocate(mem::type_info_of<int>, graph.size()));

    switch (model) {
        case StageExecutionModel::DETERMINISTIC:
            runTopologicalSort();
            createExecutionGraph();
            break;
        case StageExecutionModel::PARALLEL: {
            for (auto& node : graph) {
                emplaceExecutionNode(node);
            }
            break;
        }
        case StageExecutionModel::SERIAL: {
            runTopologicalSort();
            createExecutionGraph();
            break;
        }
        default: std::unreachable();
    }
}

void StageProcessor::runTopologicalSort() {
    std::unordered_map<int, mem::vector<int>> map;

    int* inDegrees = systemIdxToInDegrees;
    memset(inDegrees, 0, sizeof(int) * graph.size());

    for (int i = 0; i < graph.size(); ++i) {
        if (!graph[i].hasDependencies()) {
            inDegrees[i] = 0;
            map.emplace(i, mem::vector<int>{});
        }
        bool hasDependencies = false;
        for (const TypeUUID dependency : graph[i].dependencies()) {
            if (graph.findDependency(dependency)) {
                hasDependencies = true;
            }
        }
        if (!hasDependencies) {
            inDegrees[i] = 0;
            map.emplace(i, mem::vector<int>{});
        }
    }

    for (int i = 0; i < graph.size(); ++i) {
        for (int j = 0; j < graph.size(); ++j) {
            if (graph[i].dependsOn(graph[j])) {
                map[j].emplace_back(i);
                inDegrees[i]++;
            }
        }
    }
    std::queue<std::pair<int, int>> q;

    int sys = 0;
    for (const int deg : mem::make_range(inDegrees, graph.size())) {
        if (deg == 0) {
            q.emplace(sys, 0);
        }
        ++sys;
    }

    while (!q.empty()) {
        auto [system, layer] = q.front();
        q.pop();

        layers[layer].emplace_back(system);

        for (int dep : map[system]) {
            if (--inDegrees[dep] == 0) {
                q.emplace(dep, layer + 1);
            }
        }
    }
}

void StageProcessor::linkSystems(const int parentExeNode, int childExeNode) {
    tempExecutionGraphIndices[parentExeNode].emplace_back(childExeNode);
    executor.nodes[childExeNode].dependencies++;
    ++totalGraphIndices;
}

void StageProcessor::tryLinkSystems(const int currentGraphNode, const int ancestorGraphNode) {
    if (
        graph[currentGraphNode].dependsOn(graph[ancestorGraphNode])
        || graph[currentGraphNode].conflicts(graph[ancestorGraphNode])
    ) {
        linkSystems(
            systemIdxToExecutionNodeIdx[ancestorGraphNode],
            systemIdxToExecutionNodeIdx[currentGraphNode]
        );
    }
}

void StageProcessor::emplaceExecutionNode(SystemGraph::Node& node) {
    executor.nodes.emplace_back(
        UpdateSystemFnTable(*node.descriptor, node.instance),
        node.system, static_cast<StageLocalIndex>(node.descriptor->localID));

    tempExecutionGraphIndices.emplace_back(&ctx.allocator);
}

void StageProcessor::addExecutionNode(Batch* batch, const int systemIdx) {
    batch->add(systemIdx);

    systemIdxToExecutionNodeIdx[systemIdx] = static_cast<int>(executor.nodes.size());
    emplaceExecutionNode(graph.at(systemIdx));

    if (batch == batches.data()) return;

    for (const int prev : batch[-1].systems) {
        tryLinkSystems(systemIdx, prev);
    }
}

void StageProcessor::createExecutionGraph() {
    for (auto & remaining: layers | std::views::values) {
        while (!remaining.empty()) {
            auto& batch = batches.emplace_back();

            for (auto it = remaining.begin(); it != remaining.end();) {
                if (!batch.conflictsWith(graph, *it)) {
                    addExecutionNode(&batch, *it);
                    it = remaining.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    finalizeExecutionGraph();
}

void StageProcessor::finalizeExecutionGraph() {
    executor.ins = std::make_unique<int[]>(totalGraphIndices);

    int* first = executor.ins.get();

    for (size_t i = 0; i < executor.nodes.size(); ++i) {
        auto& exeNode = executor.nodes[i];
        auto& edges = tempExecutionGraphIndices[i];

        if (edges.empty()) continue;

        memcpy(first, edges.data(), edges.size() * sizeof(int));
        exeNode.outEdgesPtr = first;
        exeNode.outEdgesCount = edges.size();
        first += edges.size();
    }
}

void StageProcessor::createExecutionGraphIgnoreDependencies() {
    auto remaining = mem::clone(graph.nodes);
    batches.reserve(8 + remaining.size() / 2);

    while (!remaining.empty()) {
        auto& batch = batches.emplace_back();

        for (int i = 0; i < remaining.size(); ++i) {
            if (!batch.conflictsWith(graph, i)) {
                addExecutionNode(&batch, i);
                remaining.erase(i);
            } else {
                ++i;
            }
        }
    }
    finalizeExecutionGraph();
}

void UpdateStage::onStageBegin(Level &level) {
    if (stage->onStageBegin) {
        const Time now = SteadyTime::now();
        stage->onStageBegin(stageData, level);
        const Time elapsed = SteadyTime::now() - now;
        metrics.onStageBeginCounter += elapsed;
    }
}

void UpdateStage::onStageEnd(Level &level) {
    if (stage->onStageEnd) {
        const Time now = SteadyTime::now();
        stage->onStageEnd(stageData, level);
        const Time elapsed = SteadyTime::now() - now;
        metrics.onStageEndCounter += elapsed;
    }
}

void UpdateStage::addSystem(RuntimeSystemStageDescriptor &descriptor, const TypeUUID type, void* instance) {
    ++version;
    graph.addSystem(type, &descriptor.updateSystemDescriptor, instance);
    descriptor.updateSystemDescriptor.localID = static_cast<int>(graph.size() - 1);
}

void UpdateStage::createExecutionGraph(ExecutionGraphCreationContext& ctx) {
    version = 0;
    executor.clear();

    StageProcessor proc(ctx, graph, executor, stage->executionModel);
    results.assign(graph.size(), {});
}

void UpdateStage::dumpMermaid(const SystemKindRegistry& registry) {
    static int stageID = std::numeric_limits<int>::max();
    const int endMarker = stageID--;

    for (auto const& node : executor.nodes) {
        const auto from = registry.getFieldOf(node.systemID).descriptor->name();

        if (node.outEdges().empty()) {
            std::cout << "    " << node.systemID << "[" << from << "] " << " --> " << endMarker << "[" << this->stage->type->name << "]\n";
        }
        for (const int toID : node.outEdges()) {
            const auto toGraphIdx = executor.nodes[toID].systemID;
            const auto to = registry.getFieldOf(toGraphIdx).descriptor->name();
            std::cout << "    " << node.systemID << "[" << from << "] " << " --> " << toGraphIdx << "[" << to << "]" << "\n";
        }
    }
}

void UpdateStage::discard(const size_t graphIndex) {
    auto& toErase = graph.nodes[graphIndex];
    auto& lastNode = graph.nodes.back();

    if (graphIndex == graph.nodes.size() - 1) {
        toErase.descriptor->localID = 0;
        graph.nodes.pop_back();
    } else {
        std::swap(toErase, lastNode);
        graph.nodes.pop_back();

        graph.nodes[graphIndex].descriptor->localID = static_cast<unsigned>(graphIndex);
    }
}
