#pragma once
#include <ECS/Component/TypeUUID.h>
#include <ECS/System/RuntimeSystemDescriptor.h>

struct SystemGraph {
    struct Node {
        TypeUUID system{};
        UpdateSystemDescriptor* descriptor{};
        void* instance = nullptr;
    };
    mem::vector<Node> nodes;

    void addSystem(TypeUUID system, UpdateSystemDescriptor* descriptor, void* instance) {
        nodes.emplace_back(system, descriptor, instance);
    }

    bool findDependency(const TypeUUID dependency) {
        for (auto& [system, desc, inst] : nodes) {
            if (system == dependency) return true;
        }
        return false;
    }

    UpdateSystemDescriptor& operator[](const size_t idx) const {
        return *nodes[idx].descriptor;
    }

    Node& at(const size_t idx) {
        return nodes[idx];
    }

    size_t size() const {
        return nodes.size();
    }

    auto begin() {
        return nodes.begin();
    }

    auto end() {
        return nodes.end();
    }
};