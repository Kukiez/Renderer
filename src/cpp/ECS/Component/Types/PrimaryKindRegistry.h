#pragma once
#include <oneapi/tbb/concurrent_hash_map.h>

#include "constexpr/VariadicInfo.h"
#include "ECS/Component/ComponentRegistry.h"

struct PrimaryTypeCache {
    TypeUUID* sortedTypes;
    int* typenameSortedIndices;
    int count = 0;
};

struct TrackedComponent;

struct PrimaryComponentField {
    bool isTrackedComponent = false;

    template <typename T>
    static PrimaryComponentField of() {
        PrimaryComponentField field;
        field.isTrackedComponent = std::is_base_of_v<TrackedComponent, T>;
        return field;
    }
};

class PrimaryKindRegistry : public ComponentKindRegistry<PrimaryComponentField> {
    tbb::concurrent_hash_map<size_t, PrimaryTypeCache> rangesMap;

    const PrimaryTypeCache& createTypeCache(size_t hash, const TypeUUID* types, int count);
public:
    using ComponentKindRegistry::ComponentKindRegistry;

    template <typename... Ts>
    const PrimaryTypeCache& getTypeCache() {
        static constexpr auto PackHash = cexpr::pack_stable_hash_v<std::decay_t<Ts>...>;

        tbb::concurrent_hash_map<size_t, PrimaryTypeCache>::const_accessor accessor;
        if (!rangesMap.find(accessor, PackHash)) {
            std::array<TypeUUID, sizeof...(Ts)> types = {
                getTypeID<std::decay_t<Ts>>()...
            };
            return createTypeCache(PackHash, types.data(), static_cast<int>(types.size()));
        }
        return accessor->second;
    }

    template <typename... Ts>
    mem::range<TypeUUID> getSortedTypeRange() {
        auto& typeCache = getTypeCache<Ts...>();
        return mem::make_range(typeCache.sortedTypes, typeCache.sortedTypes + typeCache.count);
    }
};

using ArchetypeIndex = unsigned;
using ByteBufferIndex = unsigned;
using DataIndex = unsigned;