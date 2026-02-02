#pragma once
#include "SystemDescriptor.h"
#include "SystemConcept.h"
#include "SystemCallContext.h"
#include "ECS/Time.h"
#include <algorithm>
#include <ECS/Component/TypeUUID.h>

struct EmptyCallingConvention {
    template <typename Stage, typename System>
    static void invoke(Stage& stage, System& system, LevelContext& ctx, SystemCallContext& inout) {
        std::cout << "Function of Non-Runnable called: \n > Stage: " << cexpr::name_of<Stage> << "\n > System: " << cexpr::name_of<System> << std::endl;
    }
};

struct InoutCallingConvention {
    template <typename Stage, typename System>
    static void invoke(Stage& stage, System& system, LevelContext& ctx, SystemCallContext& inout) {
        static constexpr auto UpdateFnProjection = Stage::template Function<System>;
        using UpdateFnType = decltype(UpdateFnProjection);
        using FnArgs = cexpr::function_args_t<UpdateFnType>;
        using View = Stage::template StageView<System>;

        if constexpr (std::tuple_size_v<FnArgs> == 0
            || !std::is_same_v<std::decay_t<std::tuple_element_t<0, FnArgs>>, View>)
        {
            static_assert(false, "Stage::Function must be invocable with Stage::StageView<System> as first parameter");
        }

        using InputArgs = cexpr::remove_tuple_index_t<0, FnArgs>;

        View lv(ctx, &stage);

        cexpr::for_each_typename_in_tuple<InputArgs>([&]<typename... Args>(){
            if constexpr (std::is_member_function_pointer<UpdateFnType>()) {
                (system.*UpdateFnProjection)(lv,
                    static_cast<Args&>(inout.in<std::decay_t<Args>>())...
                );
            } else {
                UpdateFnProjection(lv, static_cast<Args&>(inout.in<std::decay_t<Args>>())...);
            }
        });
    }
};

template <typename Stage, typename System>
void invokeSystem(void* system, LevelContext& level, SystemCallContext& inout) {
    using CallingConvention = decltype([&] {
        if constexpr (requires
        {
            typename Stage::CallingConvention;
        }) {
            return std::type_identity<typename Stage::CallingConvention>{};
        } else {
            return std::type_identity<InoutCallingConvention>{};
        }
    }())::type;

    CallingConvention::invoke(
        *static_cast<Stage*>(inout.getStage()),
        *static_cast<System*>(system),
        level,
        inout
    );
}

struct UpdateSystemDescriptor {
    TypeUUID selfType{};

    const mem::type_info* self = nullptr;

    UpdateFn update = nullptr;

    const TypeUUID* systemReads = nullptr;
    const TypeUUID* systemWrites = nullptr;
    const TypeUUID* systemDependencies = nullptr;

    const mem::type_info* const* otherWrites = nullptr;
    const mem::type_info* const* otherReads = nullptr;

    uint16_t systemReadsCount = 0;
    uint16_t systemWritesCount = 0;
    uint16_t otherWritesCount = 0;
    uint16_t otherReadsCount = 0;
    uint16_t systemDependenciesCount = 0;

    unsigned localID = 0;

    template <IsPassiveStage Stage, typename System>
    static UpdateSystemDescriptor of() {
        UpdateSystemDescriptor system;
        system.self = mem::type_info_of<System>;
        system.update = &invokeSystem<Stage, System>;

        return system;
    }

    template <IsActiveStage Stage, typename System>
    static UpdateSystemDescriptor of() {
        using Descriptor = SystemStageDescriptor<Stage, System>;
        UpdateSystemDescriptor system;
        system.self = mem::type_info_of<System>;

        system.otherWrites = Descriptor::WriteTypes.data();
        system.otherReads = Descriptor::ReadTypes.data();

        system.otherWritesCount = Descriptor::WriteTypesCount;
        system.otherReadsCount = Descriptor::ReadTypesCount;

        system.update = &invokeSystem<Stage, System>;

        return system;
    }

    bool hasDependencies() const {
        return systemDependenciesCount > 0;
    }

    bool dependsOn(const UpdateSystemDescriptor& other) const {
        return std::ranges::any_of(mem::make_range(systemDependencies, systemDependenciesCount), [&](const auto& type) {
            return type == other.selfType;
        });
    }

    auto reads() const {
        return mem::make_range(otherReads, otherReadsCount);
    }

    auto writes() const {
        return mem::make_range(otherWrites, otherWritesCount);
    }

    auto sysReads() const {
        return mem::make_range(systemReads, systemReadsCount);
    }

    auto sysWrites() const {
        return mem::make_range(systemWrites, systemWritesCount);
    }

    auto dependencies() const {
        return mem::make_range(systemDependencies, systemDependenciesCount);
    }

    bool containsRead(const mem::type_info* type) const {
        for (const auto& read : reads()) {
            if (read == type) return true;
        }
        return false;
    }

    bool containsWrite(const mem::type_info* type) const {
        for (const auto& write : writes()) {
            if (write == type) return true;
        }
        return false;
    }

    bool containsResRead(const TypeUUID other) const {
        if (other == selfType) return true;

        for (const auto& read : sysReads()) {
            if (read == other) return true;
        }
        return false;
    }

    bool containsResWrite(const TypeUUID other) const {
        if (other == selfType) return true;

        for (const auto& write : sysWrites()) {
            if (write == other) return true;
        }
        return false;
    }

    bool conflicts(const UpdateSystemDescriptor& other) const {
        for (const auto& write : mem::make_range(otherWrites, otherWritesCount)) {
            if (other.containsRead(write) || other.containsWrite(write)) return true;
        }
        for (const auto& write : other.writes()) {
            if (this->containsRead(write)) return true;
        }

        for (const auto& write : sysWrites()) {
            if (other.containsResRead(write) || other.containsResWrite(write)) return true;
        }

        for (const auto& write : other.sysWrites()) {
            if (this->containsResRead(write)) return true;
        }
        return false;
    }
};

struct RuntimeStageDescriptor {
    SteadyTime hz = SteadyTime(0);
    StageExecutionModel executionModel;
    StageScheduleModel scheduleModel = StageScheduleModel::MANUAL;
    const mem::type_info* type;

    OnStageBegin onStageBegin = nullptr;
    OnStageEnd onStageEnd = nullptr;

    TypeUUID selfType{};

    static const RuntimeStageDescriptor* Null() {
        static RuntimeStageDescriptor desc = [] {
            RuntimeStageDescriptor desc;
            desc.type = mem::type_info_of<NullStage>;
            return desc;
        }();
        return &desc;
    }

    template <IsStage Stage>
    static constexpr RuntimeStageDescriptor of() {
        RuntimeStageDescriptor desc;

        if constexpr (requires { Stage::ExecutionModel; }) {
            desc.executionModel = Stage::ExecutionModel;

            if constexpr (Stage::ExecutionModel != StageExecutionModel::PASSIVE) {
                desc.scheduleModel = Stage::ScheduleModel;

                if constexpr (Stage::ScheduleModel == StageScheduleModel::FIXED_HZ) {
                    desc.hz = SteadyTime(Stage::Hz);
                }
            }
        } else {
            desc.executionModel = StageExecutionModel::PASSIVE;
        }

        if constexpr (requires(Stage s, Level& l) {
            s.onStageBegin(l);
        }) {
            desc.onStageBegin = [](void* s, Level& l) {
                static_cast<Stage*>(s)->onStageBegin(l);
            };
        }

        if constexpr (requires(Stage s, Level& l) {
            s.onStageEnd(l);
        }) {
            desc.onStageEnd = [](void* s, Level& l) {
                static_cast<Stage*>(s)->onStageEnd(l);
            };
        }

        desc.type = mem::type_info_of<Stage>;
        return desc;
    }
};

template <typename T>
consteval void SystemMustBeDefaultConstructible() {
    static_assert(false, __FUNCSIG__);
}

struct SystemStageDescriptorInfo {
    UpdateSystemDescriptor updateSystemDescriptor;
};

struct RuntimeSystemStageDescriptor {
    const RuntimeStageDescriptor* stage = nullptr;
    UpdateSystemDescriptor updateSystemDescriptor;

    const char* name() const {
        return updateSystemDescriptor.self->name;
    }

    size_t hash() const {
        return updateSystemDescriptor.self->hash;
    }

    bool isUpdateSystem() const {
        return updateSystemDescriptor.update;
    }

    auto dependencies() const {
        return updateSystemDescriptor.dependencies();
    }
};

struct NullSystem {};

struct RuntimeSystemDescriptor {
    RuntimeSystemStageDescriptor* stagesPtr = nullptr;
    size_t stageCount = 0;

    const mem::type_info* type = mem::type_info_of<NullSystem>;

    unsigned id{};

    RuntimeSystemDescriptor() = default;

    auto stages() const {
        return mem::make_range(stagesPtr, stageCount);
    }

    const char* name() const {
        return type->name;
    }

    size_t hash() const {
        return type->hash;
    }

    static auto& Null() {
        constexpr static RuntimeSystemDescriptor desc = [] {
            RuntimeSystemDescriptor desc;
            desc.type = mem::type_info_of<NullSystem>;
            return desc;
        }();
        return desc;
    }

    template <typename U>
    static RuntimeSystemDescriptor of() {
        RuntimeSystemDescriptor desc;
        desc.type = mem::type_info_of<U>;

        if constexpr (!std::is_default_constructible_v<U>) {
            SystemMustBeDefaultConstructible<U>();
        }
        return desc;
    }
};