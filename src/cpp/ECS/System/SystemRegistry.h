#pragma once
#include <unordered_map>
#include "RuntimeSystemDescriptor.h"
#include "SystemEnumerable.h"
#include "SystemProfileReport.h"
#include "SystemKindRegistry.h"

template <typename Stage, typename System>
void InvalidSystemForStage() {
    static_assert(false, __FUNCSIG__);
}

template <typename Stage, typename RootSystem, typename System>
constexpr bool isCycle() {
    if constexpr (!IsValidSystemForStage<Stage, System>) {
    } else {
        using Dependencies = SystemStageDescriptor<Stage, System>::hard_deps;
        if constexpr (
            cexpr::is_typename_in_tuple_v<RootSystem, Dependencies>
        ) {
            static_assert(false, "Cyclic Dependency Reference Detected in RootSystem");
        } else {
            cexpr::for_each_typename_in_tuple<Dependencies>([]<typename... Ts>() {
                return (isCycle<Stage, RootSystem, Ts>() || ...);
            });
        }
    }
    return false;
}

struct StageEntry {
    UpdateStage* stagePtr;
    mem::vector<SystemEnumerable> systemLocalData;

    UpdateStage* operator -> () const {
        return stagePtr;
    }

    UpdateStage& operator * () const {
        return *stagePtr;
    }

    operator UpdateStage& () const {
        return *stagePtr;
    }

    template <typename T, typename ConstructCallback>
    void addEnumerable(ConstructCallback&& cb) {
        auto& data = systemLocalData.emplace_back();
        new (&data) SystemEnumerable(SystemEnumerable::of<T>(std::forward<ConstructCallback>(cb)));
    }

    void initialize() {
        for (auto& enumerable : systemLocalData) {
            if (enumerable.size() != 0) {
                enumerable.reinitialize(stagePtr->size());
            } else {
                enumerable.initialize(stagePtr->size());
            }
        }
    }

    SystemEnumerable* getEnumerable(const mem::type_info* enumerableType) {
        for (auto& enumerable : systemLocalData) {
            if (enumerable.type == enumerableType) {
                return &enumerable;
            }
        }
        return nullptr;
    }
};

struct StagesViewEnd {};

class StageView {
    const UpdateStage* stage;
public:
    StageView(const UpdateStage* stage) : stage(stage) {}

    StageProfileReport getProfileReport() const {
        return StageProfileReport{stage};
    }
};

class StagesViewIterator {
    const StageEntry* first;
    const StageEntry* last;
public:
    StagesViewIterator(const StageEntry* first, const StageEntry* last) : first(first), last(last) {}

    bool operator==(const StagesViewEnd&) const {
        return first == last;
    }

    bool operator!=(const StagesViewEnd&) const {
        return first != last;
    }

    StageView operator*() const {
        return StageView(first->stagePtr);
    }

    StageView operator->() const {
        return StageView(first->stagePtr);
    }

    void increment() {
        ++first;
    }

    void decrement() {
        --first;
    }

    StagesViewIterator& operator++() {
        increment();
        return *this;
    }

    StagesViewIterator operator ++ (int) {
        auto copy = *this;
        increment();
        return copy;
    }

    StagesViewIterator& operator--() {
        decrement();
        return *this;
    }

    StagesViewIterator operator -- (int) {
        auto copy = *this;
        decrement();
        return copy;
    }
};

class StagesViewIteratable {
    const StageEntry* first;
    const StageEntry* last;
public:
    StagesViewIteratable(const StageEntry* first, const StageEntry* last) : first(first), last(last) {}

    auto begin() const {
        return StagesViewIterator(first, last);
    }

    static auto end() {
        return StagesViewEnd{};
    }
};

struct SystemErrorStack {

};

class SystemRegistry {
    friend class SystemQuery;
    friend class SystemInfo;

    mem::vector<StageEntry> stages;

    mem::vector<UpdateStage*> perTickStages;
    mem::vector<UpdateStage*> manualStages;

    SystemKindRegistry systemKindRegistry;
    StageKindRegistry stageKindRegistry;

    mem::byte_arena<> updateStageAllocator = mem::create_byte_arena(sizeof(UpdateStage) * 5);
public:
    ECSAPI SystemRegistry();

    SystemRegistry(const SystemRegistry&) = delete;
    SystemRegistry(SystemRegistry&&) = delete;

    ~SystemRegistry() {
        for (auto& [stagePtr, systemLocalData] : stages) {
            stagePtr->~UpdateStage();
        }
    }

    template <typename Stage, typename Enumerable, typename... Args>
    requires std::is_default_constructible_v<Enumerable>
    bool addSystemEnumerable(Args&&... args) {
        if (!stageKindRegistry.has<Stage>()) return false;
        auto& stage = getUpdateStage<Stage>();

        stage.template addEnumerable<Enumerable>([args...](void* enumerable, const size_t count) mutable {
            auto* enumerablePtr = (Enumerable*)enumerable;
            for (size_t i = 0; i < count; ++i) {
                new (enumerablePtr + i) Enumerable(args...);
            }
        });
        return true;
    }

    template <typename System, typename... Detectors>
    void addSystem() {
        using Descriptor = SystemDescriptor<System, Detectors...>;

        cexpr::for_each_typename_in_tuple<typename Descriptor::stages>([]<typename... Stage>(){
            (isCycle<Stage, System, System>(), ...);
        });
        if (systemKindRegistry.addSystem<System, Descriptor>(stageKindRegistry)) {
            const ComponentField<SystemField>& field = systemKindRegistry.getFieldOf<System>();
            for (auto& stageSystemDesc : field.descriptor->stages()) {
                const auto& stage = stages[stageSystemDesc.stage->selfType.id()];
                stage->addSystem(stageSystemDesc, systemKindRegistry.findSystem<System>(), field.instance);
            }
        } else {
            assert(false);
        }
    }

    ECSAPI void addStage(const ComponentField<StageField>& field, TypeUUID type);

    template <typename Stage, typename... Args>
    Stage& addStage(Args&&... args) {
        ComponentField<StageField>& field = stageKindRegistry.addStage<Stage>(std::forward<Args>(args)...);

        const TypeUUID type = stageKindRegistry.findStage<Stage>();
        addStage(field, type);
        return *static_cast<Stage*>(field.instance);
    }

    template <typename T>
    bool hasStage() const {
        return stageKindRegistry.has<T>();
    }

    template <typename Sys, typename Stage>
    bool hasSystem() {
        if (!systemKindRegistry.has<Sys>()) return false;

        auto desc = systemKindRegistry.getFieldOf<Sys>().descriptor;

        for (auto& stage : desc->stages()) {
            if (stage.stage->hash == cexpr::type_hash_v<Stage>) {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    T& getStage() {
        return *static_cast<T*>(stageKindRegistry.getFieldOf<T>().instance);
    }

    template <typename Stage>
    StageEntry& getUpdateStage() {
        if (!stageKindRegistry.has<Stage>()) {
            assert(false);
        }
        return stages[stageKindRegistry.findStage<Stage>().id()];
    }

    template <typename T>
    T& getSystem() {
        if (!systemKindRegistry.has<T>()) {
            assert(false);
        }
        return *static_cast<T*>(systemKindRegistry.getFieldOf<T>().instance);
    }

    ECSAPI SystemProfileReport getProfileReport(const RuntimeSystemDescriptor& desc);

    SystemProfileReport getProfileReport(const TypeUUID system) {
        const auto& desc = *systemKindRegistry.getFieldOf(system).descriptor;
        return getProfileReport(desc);
    }

    StageProfileReport getStageProfileReport(const TypeUUID stage) {
        return StageProfileReport{stages[stage.id()].stagePtr};
    }

    ECSAPI bool doSystemDependenciesExist(UpdateSystemDescriptor& descriptor);

    ECSAPI SystemErrorStack createExecutionGraph(StageEntry& stageEntry);

    ECSAPI void createExecutionGraphs();

    auto& getUpdateStages() {
        return stages;
    }

    auto& getPerTickStages() {
        return perTickStages;
    }

    auto& getManualStages() {
        return manualStages;
    }

    auto getAllRegisteredStages() const {
        return StagesViewIteratable(stages.data(), stages.data() + stages.size());
    }

    auto& getSystemKindRegistry(this auto&& self) {
        return self.systemKindRegistry;
    }

    auto& getStageKindRegistry(this auto&& self) {
        return self.stageKindRegistry;
    }
};