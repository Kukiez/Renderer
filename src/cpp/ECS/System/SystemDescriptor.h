#pragma once

#include "ISystem.h"
#include "Stages/StageInterface.h"
#include <memory/vector.h>

#include "constexpr/FunctionInfo.h"
#include "constexpr/PackManip.h"
#include "constexpr/VariadicManip.h"

struct InoutCallingConvention;

template <typename System, typename Component>
consteval bool NotDeclaredAsReadOrWrite() {
    static_assert(false, __FUNCSIG__);
}

template <typename Stage>
struct StageDescriptor {
    static constexpr StageExecutionModel ExecutionModel = [] {
        if constexpr (requires
        {
            Stage::ExecutionModel;
        }) {
            return Stage::ExecutionModel;
        } else {
            return StageExecutionModel::PASSIVE;
        }
    }();
};

struct DescriptorFilter {
    template <typename T, typename Result>
    struct IsPassiveStage {
        static constexpr bool value = StageDescriptor<T>::ExecutionModel == StageExecutionModel::PASSIVE;
    };

    template <typename T, typename Result>
    struct IsNotPassiveStage {
        static constexpr bool value = StageDescriptor<T>::ExecutionModel != StageExecutionModel::PASSIVE;
    };

    template <typename List, typename Result, size_t I>
    struct OnPass {
        using result = cexpr::add_typename_to_tuple_t<std::decay_t<std::tuple_element_t<I, List>>, Result>;
    };

    template <typename Stages>
    using PassiveStages = cexpr::filter_typenames_t<Stages, IsPassiveStage, OnPass, cexpr::noop>;

    template <typename Stages>
    using ActiveStages = cexpr::filter_typenames_t<Stages, IsNotPassiveStage, OnPass, cexpr::noop>;
};

template <typename... Stages>
struct StageDetector {
    template <typename System>
    struct SystemCheck {
        template <typename Stage, typename Result>
        struct ForStage {
            static constexpr bool value = Stage::template HasFunction<System>;
        };
    };

    template <typename System>
    using detect = cexpr::filter_typenames_t<
        std::tuple<Stages...>,
        SystemCheck<System>::template ForStage,
        cexpr::append_typename,
        cexpr::noop
    >;
};

template <typename System>
struct ResourceSystemDescriptor {
    template <typename T>
    static constexpr auto getConflictModeOf() {
        if constexpr (requires {
            { T::getConflictMode() } -> std::same_as<ConflictMode>;
        }) {
            return T::getConflictMode();
        } else {
            return static_cast<void *>(nullptr);
        }
    }

    static constexpr auto conflictMode = getConflictModeOf<System>();
    static constexpr auto IsResourceSystem = !std::is_same_v<std::decay_t<decltype(conflictMode)>, void*>;

    template <ConflictMode Mode>
    consteval static bool isConflictModeEqual() {

        if constexpr (std::is_same_v<std::decay_t<decltype(conflictMode)>, void*>) {
            return false;
        } else if constexpr (conflictMode == Mode) {
            return true;
        }
        return false;
    }
};

template <typename System, typename... Detectors>
struct SystemDescriptor {
    template <typename... Ts>
    constexpr static auto GetStages(Stages<Ts...>*) {
        return std::type_identity<std::tuple<Ts...>>{};
    }
    template <typename... Ts>
    constexpr static auto GetStages(...) {
        return std::type_identity<std::tuple<>>{};
    }

    using all_stages = cexpr::remove_duplicates<cexpr::tuple_join_t<
        typename decltype(GetStages(cexpr::declpointer<System>()))::type,
        typename Detectors::template detect<System>...
    >>;

    using stages = all_stages;

    static constexpr auto StageCount = std::tuple_size_v<stages>;
};

template <typename Stage>
concept IsStage = cexpr::is_base_of_template<::Stage, Stage>;

template <typename Stage>
concept IsActiveStage = IsStage<Stage> && StageDescriptor<Stage>::ExecutionModel != StageExecutionModel::PASSIVE;

template <typename Stage>
concept IsPassiveStage = IsStage<Stage> && StageDescriptor<Stage>::ExecutionModel == StageExecutionModel::PASSIVE;

struct AllAccessSystem {};

#define FRIEND_DESCRIPTOR \
    template <typename, typename> friend struct SystemStageDescriptor; \
    template <typename, typename...> friend struct SystemDescriptor;             \
    template <typename> friend struct ResourceSystemDescriptor;

template <typename, typename>
struct SystemStageDescriptor;

template <typename Stage, typename System>
requires (std::is_same_v<System, AllAccessSystem>)
struct SystemStageDescriptor<Stage, System> {
    constexpr static bool AllAccess = true;

    template <typename>
    static constexpr bool CanWriteComponent() {
        return true;
    }

    template <typename>
    static constexpr bool CanReadComponent() {
        return true;

    }
    template <typename>
    constexpr static bool CanAccessResMut() {
        return true;
    }

    template <typename>
    constexpr static bool CanAccessResConst() {
        return true;
    }

    template <typename T>
    using SelectConstness = std::decay_t<T>;
};

template <typename Stage>
concept IsInoutCallingConvention = [] {
    if constexpr (requires
    {
        typename Stage::CallingConvention;
    }) {
        return std::is_same_v<typename Stage::CallingConvention, InoutCallingConvention>;
    } else {
        return true;
    }
}();


template <typename Stage, typename System>
requires (!std::is_same_v<System, AllAccessSystem>) && IsInoutCallingConvention<Stage>
struct SystemStageDescriptor<Stage, System> {
    constexpr static bool AllAccess = false;

    template <template <typename...> typename Test, typename... Ts>
    constexpr static auto GetBaseTypenames(Test<Ts...>*) {
        return std::type_identity<std::tuple<Ts...>>{};
    }

    template <template <typename...> typename, typename...>
    constexpr static auto GetBaseTypenames(...) {
        return std::type_identity<std::tuple<>>{};
    }

    template <template <typename...> typename StageMeta, template <typename...> typename Meta>
    using Metadata = cexpr::remove_duplicates<cexpr::tuple_join_t<
        typename decltype(GetBaseTypenames<Meta>(cexpr::declpointer<System>()))::type,
        typename decltype(GetBaseTypenames<StageMeta>(cexpr::declpointer<System>()))::type
    >>;

    using DependenciesFromBase = Metadata<Stage::template Dependencies, Dependencies>;
    using WritesFromBase = Metadata<Stage::template Writes, Writes>;
    using ReadsFromBase = Metadata<Stage::template Reads, Reads>;

    using stage = Stage;

    using UpdateFn = decltype(stage::template Function<System>);
    using UpdateFnReturn = cexpr::function_return_t<UpdateFn>;
    using UpdateFnArgs = cexpr::function_args_t<UpdateFn>;

    using hard_deps = DependenciesFromBase;

    using reads = ReadsFromBase;
    using writes = WritesFromBase;

    using resReads = decltype([] {
        if constexpr (std::tuple_size_v<UpdateFnArgs> <= 1) {
            return std::type_identity<std::tuple<>>{};
        } else {
            using Args = cexpr::remove_tuple_index_t<0, UpdateFnArgs>;
            return std::type_identity<cexpr::filter_typenames_t<Args, cexpr::predicate<std::is_const>::op, cexpr::append_decayed_typename, cexpr::noop>>{};
        }
    }())::type;

    using resWrites = decltype([] {
        if constexpr (std::tuple_size_v<UpdateFnArgs> <= 1) {
            return std::type_identity<std::tuple<>>{};
        } else {
            using Args = cexpr::remove_tuple_index_t<0, UpdateFnArgs>;
            return std::type_identity<cexpr::filter_typenames_t<Args, cexpr::opposite<std::is_const>::op, cexpr::append_decayed_typename, cexpr::noop>>{};
        }
    }())::type;

    using resInputs = cexpr::remove_tuple_index_t<0, UpdateFnArgs>;

    static constexpr auto SystemInputsCount = std::tuple_size_v<resInputs>;
    static constexpr auto WriteTypesCount = std::tuple_size_v<writes>;
    static constexpr auto ReadTypesCount = std::tuple_size_v<reads>;

    static constexpr auto WriteTypes = []<size_t... Is>(std::index_sequence<Is...>) {
        return std::array<const mem::type_info*, WriteTypesCount>{
            mem::type_info_of<std::tuple_element_t<Is, writes>>...
        };
    }(std::make_index_sequence<std::tuple_size_v<writes>>{});

    static constexpr auto ReadTypes = []<size_t... Is>(std::index_sequence<Is...>) {
        return std::array<const mem::type_info*, ReadTypesCount>{
            mem::type_info_of<std::tuple_element_t<Is, reads>>...
        };
    }(std::make_index_sequence<std::tuple_size_v<reads>>{});

    template <typename T>
    static constexpr bool CanWriteComponent() {
        return cexpr::is_typename_in_tuple_v<T, writes> || cexpr::is_typename_in_tuple_v<T, resWrites>;
    }

    template <typename T>
    static constexpr bool CanReadComponent() {
        return cexpr::is_typename_in_tuple_v<T, reads> || cexpr::is_typename_in_tuple_v<T, writes> || cexpr::is_typename_in_tuple_v<T, resReads> || cexpr::is_typename_in_tuple_v<T, resWrites>;
    }

    template <typename T>
    using SelectConstness = decltype([] {
        using Type = std::decay_t<T>;
        if constexpr (CanWriteComponent<Type>()) {
            return std::type_identity<Type>{};
        } else if constexpr (CanReadComponent<Type>()) {
            return std::type_identity<const Type>{};
        } else {
            NotDeclaredAsReadOrWrite<System, T>();
        }
    }())::type;

    template <typename TSys>
    static constexpr size_t getInputSystemIndex() {
        return cexpr::find_tuple_typename_index_v<TSys, UpdateFnArgs> - 1;
    }
};

template <typename Stage, typename System>
requires (!std::is_same_v<System, AllAccessSystem>) && (!IsInoutCallingConvention<Stage>)
struct SystemStageDescriptor<Stage, System> {
    constexpr static bool AllAccess = false;

    template <template <typename...> typename Test, typename... Ts>
    constexpr static auto GetBaseTypenames(Test<Ts...>*) {
        return std::type_identity<std::tuple<Ts...>>{};
    }

    template <template <typename...> typename, typename...>
    constexpr static auto GetBaseTypenames(...) {
        return std::type_identity<std::tuple<>>{};
    }

    template <template <typename...> typename StageMeta, template <typename...> typename Meta>
    using Metadata = cexpr::remove_duplicates<cexpr::tuple_join_t<
        typename decltype(GetBaseTypenames<Meta>(cexpr::declpointer<System>()))::type,
        typename decltype(GetBaseTypenames<StageMeta>(cexpr::declpointer<System>()))::type
    >>;

    using DependenciesFromBase = Metadata<Stage::template Dependencies, Dependencies>;
    using WritesFromBase = Metadata<Stage::template Writes, Writes>;
    using ReadsFromBase = Metadata<Stage::template Reads, Reads>;

    using stage = Stage;

    using hard_deps = DependenciesFromBase;

    using reads = ReadsFromBase;
    using writes = WritesFromBase;

    using resReads = std::tuple<>;
    using resWrites = std::tuple<>;

    using resInputs = std::tuple<>;

    static constexpr auto SystemInputsCount = 0;
    static constexpr auto WriteTypesCount = std::tuple_size_v<writes>;
    static constexpr auto ReadTypesCount = std::tuple_size_v<reads>;

    static constexpr auto WriteTypes = []<size_t... Is>(std::index_sequence<Is...>) {
        return std::array<const mem::type_info*, WriteTypesCount>{
            mem::type_info_of<std::tuple_element_t<Is, writes>>...
        };
    }(std::make_index_sequence<std::tuple_size_v<writes>>{});

    static constexpr auto ReadTypes = []<size_t... Is>(std::index_sequence<Is...>) {
        return std::array<const mem::type_info*, ReadTypesCount>{
            mem::type_info_of<std::tuple_element_t<Is, reads>>...
        };
    }(std::make_index_sequence<std::tuple_size_v<reads>>{});

    static size_t totalReads() {
        return std::tuple_size_v<reads>;
    }

    static size_t totalWrites() {
        return std::tuple_size_v<writes>;
    }

    static size_t totalDependencies() {
        return std::tuple_size_v<hard_deps>;
    }

    template <typename T>
    static constexpr bool CanWriteComponent() {
        return cexpr::is_typename_in_tuple_v<T, writes> || cexpr::is_typename_in_tuple_v<T, resWrites>;
    }

    template <typename T>
    static constexpr bool CanReadComponent() {
        return cexpr::is_typename_in_tuple_v<T, reads> || cexpr::is_typename_in_tuple_v<T, writes> || cexpr::is_typename_in_tuple_v<T, resReads> || cexpr::is_typename_in_tuple_v<T, resWrites>;
    }

    template <typename T>
    using SelectConstness = decltype([] {
        using Type = std::decay_t<T>;
        if constexpr (CanWriteComponent<Type>()) {
            return std::type_identity<Type>{};
        } else if constexpr (CanReadComponent<Type>()) {
            return std::type_identity<const Type>{};
        } else {
            NotDeclaredAsReadOrWrite<System, T>();
        }
    }())::type;
};

template <typename System>
void for_each_stage(auto&& cbx) {
    cexpr::for_each_typename_in_tuple<typename SystemDescriptor<System>::stages>(cbx);
}

template <typename Stage, typename System>
concept IsValidSystemForStage = cexpr::is_typename_in_tuple_v<Stage, typename SystemDescriptor<System>::stages> && requires {
    Stage::template HasFunction<System>;
};