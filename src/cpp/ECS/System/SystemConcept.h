#pragma once
#include "SystemDescriptor.h"
#include "constexpr/Traits.h"

struct LevelContext;
class SystemCallContext;

class Level;

using UpdateFn = void(*)(void*, LevelContext&, SystemCallContext&);

using OnStageBegin = void(*)(void*, Level&);
using OnStageEnd = void(*)(void*, Level&);

template <typename System>
concept HasUpdateFn = true;

template <typename S>
concept IsResourceSystem = ResourceSystemDescriptor<S>::IsResourceSystem;

template <typename S>
concept IsSystem = true;

template <typename S>
concept IsSystemPack = requires {
    typename S::Pack;
};
