#pragma once
#include "Stages/StageInterface.h"

template <typename... Ws>
struct Writes {
    using writes = std::tuple<Ws...>;
};

template <typename... Rs>
struct Reads {
    using reads = std::tuple<Rs...>;
};

template <typename... Ds>
struct HardDependencies {
    using hard_deps = std::tuple<Ds...>;
};

template <typename... Ds>
using Dependencies = HardDependencies<Ds...>;

template <typename... Ts>
struct Stages {
    using stages = std::tuple<Ts...>;
};

enum class ConflictMode {
    EXCLUSIVE, /* Only one Thread can write to this Resource */
    SHARED,    /* Multiple Threads can write to this Resource at once */
};

template <ConflictMode Mode = ConflictMode::EXCLUSIVE>
struct ResourceSystem {
    consteval static ConflictMode getConflictMode() {
        return Mode;
    }
};

struct NullStage : Stage<NullStage> {};

template <typename... Ss>
struct ReadsResources  {
    using res_reads = std::tuple<Ss...>;
};

template <typename... Ss>
struct WritesResources {
    using res_writes = std::tuple<Ss...>;
};

struct SystemPack {
    template <typename System>
    constexpr auto getPack(this System) {
        constexpr static bool HasPackMember = requires {
            typename System::Pack;
        };
        static_assert(HasPackMember, "SystemPack must have a nested ' using Pack = std::tuple<Systems...> '");
        return std::type_identity<typename System::Pack>{};
    }
};