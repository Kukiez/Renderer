#pragma once
#include "SystemRegistry.h"

class Level;

class SystemInfo {
    SystemRegistry& registry;
    TypeUUID system;
public:
    SystemInfo(SystemRegistry& registry, TypeUUID system) : registry(registry), system(system) {}

    StageLocalIndex getStageLocalIndex(TypeUUID stage) const;

    bool isInStage(TypeUUID stage) const;

    bool isValid() const {
        return system.kind() == registry.getSystemKindRegistry().getKind() && system.id() != 0;
    }

    template <IsStage Stage>
    bool isInStage() const {
        return isInStage(registry.getStageKindRegistry().findStage<Stage>());
    }

    std::string_view name() const;

    TypeUUID type() const {
        return system;
    }

    void* instance() const {
        return registry.getSystemKindRegistry().getFieldOf(system).instance;
    }
};

class StageInfo {
    SystemRegistry& registry;
    TypeUUID stage{};
public:
    StageInfo(SystemRegistry& registry, const TypeUUID stage) : registry(registry), stage(stage) {}

    std::string_view name() const;

    size_t hash() const;
};

class AbstractStageReference {
    SystemEnumerable* getEnumerableImpl(const mem::type_info* enumType) const;
    void* getSystemEnumerableImpl(TypeUUID system, const mem::type_info* enumType) const;

    SystemRegistry& registry;
    TypeUUID stage{};
public:
    AbstractStageReference(Level& level, ComponentIndex cIndex);
    AbstractStageReference(Level& level, TypeUUID type);
    AbstractStageReference(LevelContext& level, TypeUUID type);
    AbstractStageReference(SystemRegistry& registry, const TypeUUID stage) : registry(registry), stage(stage) {}

    template <IsStage Stage>
    static AbstractStageReference of(SystemRegistry& registry) {
        return {registry, registry.getStageKindRegistry().findStage<Stage>()};
    }

    bool isValid() const {
        return stage.kind() == registry.getStageKindRegistry().getKind() && stage.id() != 0;
    }

    void* instance() const;

    bool invoke(Level& level, TypeUUID system, SystemInvokeParams invokeParams = {}) const;

    template <IsSystem System>
    bool invoke(Level& level, const SystemInvokeParams invokeParams = {}) const {
        return invoke(level, registry.getSystemKindRegistry().findSystem<System>(), invokeParams);
    }

    template <IsSystem System, typename Param>
    bool invoke(Level& level, Param&& param) const {
        return invoke(level, registry.getSystemKindRegistry().findSystem<System>(), SystemInvokeParams::of(&param));
    }

    void run(Level& level, SystemInvokeParams invokeParams = {}) const;

    bool hasSystem(TypeUUID system) const;

    template <IsSystem System>
    bool hasSystem() const {
        return hasSystem(registry.getSystemKindRegistry().findSystem<System>());
    }

    size_t getSystemCount() const;

    template <typename Enumerable>
    Enumerable* getEnumerableFor(const TypeUUID system) {
        return static_cast<Enumerable*>(getSystemEnumerableImpl(system, mem::type_info_of<Enumerable>));
    }

    template <typename Enumerable, typename System>
    Enumerable* getEnumerableFor() {
        return getEnumerableFor<Enumerable>(registry.getSystemKindRegistry().findSystem<System>());
    }

    template <typename Enumerable>
    SystemEnumerableView<Enumerable> getEnumerable() {
        return SystemEnumerableView<Enumerable>{getEnumerableImpl(mem::type_info_of<Enumerable>)};
    }

    template <typename Enumerable>
    bool hasEnumerable() const {
        return getEnumerableImpl(mem::type_info_of<Enumerable>) != nullptr;
    }

    StageInfo info() const {
        return {registry, stage};
    }

    TypeUUID type() const {
        return stage;
    }

    SystemInfo getSystemAt(StageLocalIndex slIndex) const;
};

template <IsStage Stage>
class StageReference : public AbstractStageReference {
public:
    explicit StageReference(Level& level) : AbstractStageReference(level,
        ComponentTypeRegistry::getOrCreateComponentIndex<Stage>(StageComponentType::Kind)
    ) {}
    
    explicit StageReference(const AbstractStageReference ref) : AbstractStageReference(ref) {}

    using AbstractStageReference::AbstractStageReference;

    void* instance() = delete;

    Stage* instance() const {
        return static_cast<Stage*>(AbstractStageReference::instance());
    }

    Stage* operator -> () const {
        return static_cast<Stage*>(AbstractStageReference::instance());
    }

    Stage& operator * () const {
        return *static_cast<Stage*>(AbstractStageReference::instance());
    }
};

class AbstractSystemReference {
    SystemRegistry& registry;
    TypeUUID system{};
public:
    AbstractSystemReference(SystemRegistry& registry, TypeUUID system) : registry(registry), system(system) {}

    void* instance() const;

    bool isValid() const;
};

template <IsSystem System>
class SystemReference {
    SystemRegistry& registry;
    TypeUUID system;
public:
    SystemReference(SystemRegistry& registry) : registry(registry), system(registry.getSystemKindRegistry().findSystem<System>()) {}

    System* operator -> () const {
        return static_cast<System*>(AbstractSystemReference{registry, system}.instance());
    }

    System& operator * () const {
        return *static_cast<System*>(AbstractSystemReference{registry, system}.instance());
    }
};

class SystemQuery {
    SystemRegistry& registry;
public:
    explicit SystemQuery(Level& level);
    explicit SystemQuery(LevelContext& level);
    explicit SystemQuery(SystemRegistry& registry) : registry(registry) {}

    SystemInfo getSystemInfo(const TypeUUID system) const {
        return {registry, system};
    }

    SystemInfo getSystemInfo(const ComponentIndex cIndex) const {
        return getSystemInfo(registry.getSystemKindRegistry().getTypeID(cIndex));
    }

    template <IsSystem System>
    SystemReference<System> getSystem() const {
        return {registry};
    }

    template <IsSystem System>
    SystemInfo getSystemInfo() const {
        return getSystemInfo(registry.getSystemKindRegistry().findSystem<System>());
    }

    template <IsStage Stage>
    TypeUUID getStageTypeID() const {
        return registry.getStageKindRegistry().findStage<Stage>();
    }

    AbstractStageReference getStageReference(const TypeUUID stage) const {
        return {registry, stage};
    }

    template <IsStage Stage>
    StageReference<Stage> getStageReference() const {
        return StageReference<Stage>{getStageReference(registry.getStageKindRegistry().findStage<Stage>())};
    }
};;