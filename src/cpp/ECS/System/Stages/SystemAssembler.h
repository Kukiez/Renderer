#pragma once
#include "../SystemConcept.h"

template <typename... Detectors>
class SystemAssembler {
    SystemRegistry& registry;
public:
    SystemAssembler(SystemRegistry& registry) : registry(registry) {}

    template <IsSystem S>
    TypeUUID addSystem() {
        if constexpr (IsSystemPack<S>) {
            cexpr::for_each_typename_in_tuple<typename S::Pack>([&]<typename... Ss>() {
                (registry.addSystem<Ss, Detectors...>(), ...);
            });
        } else {
            registry.addSystem<S, Detectors...>();
        }
        return registry.getSystemKindRegistry().findSystem<S>();
    }
};