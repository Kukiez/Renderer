#include "SystemQuery.h"

#include "ECS/Level/Level.h"


StageLocalIndex SystemInfo::getStageLocalIndex(const TypeUUID stage) const {
    if (stage.kind() != registry.getStageKindRegistry().getKind()) {
        return StageLocalIndex::INVALID;
    }
    auto& field = registry.getSystemKindRegistry().getFieldOf(system);

    if (!field) {
        return StageLocalIndex::INVALID;
    }

    for (auto& [stageDesc, updateDesc] : field.descriptor->stages()) {
        if (stageDesc->selfType == stage) {
            return static_cast<StageLocalIndex>(updateDesc.localID);
        }
    }
    return StageLocalIndex::INVALID;
}

bool SystemInfo::isInStage(const TypeUUID stage) const {
    return getStageLocalIndex(stage) != StageLocalIndex::INVALID;
}

std::string_view SystemInfo::name() const {
    return registry.getSystemKindRegistry().getFieldOf(system).type->name;
}

bool AbstractStageReference::invoke(Level &level, const TypeUUID system, const SystemInvokeParams invokeParams) const {
    if (!isValid()) return false;

    const SystemInfo sInfo(registry, system);

    const auto index = sInfo.getStageLocalIndex(stage);

    if (index == StageLocalIndex::INVALID) return false;

    const auto& graphNode = registry.getUpdateStages()[stage.id()].stagePtr->graph.at(static_cast<unsigned>(index));

    SystemCallContext invocationCtx(registry.getSystemKindRegistry(), invokeParams);
    invocationCtx.setStage(instance());

    graphNode.descriptor->update(graphNode.instance, level.internal(), invocationCtx);

    return true;
}

void AbstractStageReference::run(Level &level, SystemInvokeParams invokeParams) const {
    if (!isValid()) return;

    auto& [stagePtr, systemLocalData] = registry.getUpdateStages()[stage.id()];
    level.getRuntime().runUpdateStage(*stagePtr, level, invokeParams);
}

bool AbstractStageReference::hasSystem(TypeUUID system) const {
    const SystemInfo sInfo(registry, system);
    return sInfo.isInStage(stage);
}

std::string_view StageInfo::name() const {
    return registry.getStageKindRegistry().getFieldOf(stage).type->name;
}

size_t StageInfo::hash() const {
    return registry.getStageKindRegistry().getFieldOf(stage).type->hash;
}

SystemEnumerable * AbstractStageReference::getEnumerableImpl(const mem::type_info *enumType) const {
    return registry.getUpdateStages()[stage.id()].getEnumerable(enumType);
}

void * AbstractStageReference::getSystemEnumerableImpl(const TypeUUID system, const mem::type_info *enumType) const {
    if (!isValid()) return nullptr;

    const SystemInfo sInfo(registry, system);

    const auto index = sInfo.getStageLocalIndex(stage);

    if (index == StageLocalIndex::INVALID) return nullptr;

    const auto enumerable = getEnumerableImpl(enumType);

    if (!enumerable) return nullptr;

    return enumerable->get(static_cast<size_t>(index));
}

AbstractStageReference::AbstractStageReference(Level& level, ComponentIndex cIndex) :
registry(level.internal().systemRegistry), stage(level.internal().systemRegistry.getStageKindRegistry().getTypeID(cIndex)) {}

AbstractStageReference::AbstractStageReference(Level &level, const TypeUUID type) : registry(level.internal().systemRegistry), stage(type) {

}

AbstractStageReference::AbstractStageReference(LevelContext &level, TypeUUID type) : registry(level.systemRegistry), stage(type) {
}

void * AbstractStageReference::instance() const {
    if (!isValid()) return nullptr;
    return registry.getUpdateStages()[stage.id()].stagePtr->stageData;
}

size_t AbstractStageReference::getSystemCount() const {
    if (!isValid()) return 0;
    return registry.getUpdateStages()[stage.id()].stagePtr->graph.size();
}

SystemInfo AbstractStageReference::getSystemAt(StageLocalIndex slIndex) const {
    if (!isValid()) return SystemInfo(registry, TypeUUID::of(SystemComponentType::Kind, 0));

    const int idx = static_cast<int>(slIndex);

    auto& graph = registry.getUpdateStages()[stage.id()].stagePtr->graph;

    if (graph.size() <= idx) return SystemInfo(registry, TypeUUID::of(SystemComponentType::Kind, 0));
    return SystemInfo(registry, graph[idx].selfType);
}

void * AbstractSystemReference::instance() const {
    return registry.getSystemKindRegistry().getFieldOf(system).instance;
}

bool AbstractSystemReference::isValid() const {
    return system.kind() == registry.getSystemKindRegistry().getKind() && system.id() != 0;
}

SystemQuery::SystemQuery(Level &level) : registry(level.internal().systemRegistry) {

}

SystemQuery::SystemQuery(LevelContext &level) : registry(level.systemRegistry) {
}
