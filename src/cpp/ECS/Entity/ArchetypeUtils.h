#pragma once
#include "Archetype.h"
#include <memory/vector.h>
#include <vector>

struct ArchetypeTransitionKey {
    size_t adds = 0;
    size_t removes = 0;

    bool operator == (const ArchetypeTransitionKey& other) const {
        return adds == other.adds && removes == other.removes;
    }

    bool operator != (const ArchetypeTransitionKey& other) const {
        return adds != other.adds && removes != other.removes;
    }

    bool operator < (const ArchetypeTransitionKey& other) const {
        return (adds < other.adds) || (removes < other.removes && adds == other.adds);
    }
};

struct PrimaryArchetype {
    Archetype archetype;
    mem::vector<std::pair<ArchetypeTransitionKey, ArchetypeIndex>> transitionEdges;

    auto lowerBound(const ArchetypeTransitionKey& key) {
        return std::lower_bound(
            transitionEdges.begin(), transitionEdges.end(), key,
            [](const std::pair<ArchetypeTransitionKey, ArchetypeIndex>& a, const ArchetypeTransitionKey& b) {
                return a.first < b;
            }
        );
    }

    void addEdge(const ArchetypeTransitionKey& key, ArchetypeIndex archetype) {
        auto it = lowerBound(key);
        transitionEdges.emplace(it, std::make_pair(key, archetype));
    }

    ArchetypeIndex findEdge(const ArchetypeTransitionKey& key) {
        if (auto it = lowerBound(key); it != transitionEdges.end()) {
            return it->second;
        }
        return INVALID_INDEX<ArchetypeIndex>;
    }

    PrimaryArchetype() = default;
    PrimaryArchetype(Archetype&& archetype) : archetype(std::move(archetype)) {}
};

struct TemporaryArchetype {
    ArchetypeIndex index;
    Archetype archetype;
    size_t hash;
};