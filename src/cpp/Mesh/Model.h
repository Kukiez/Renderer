#pragma once
#include "ModelCooking.h"
#include "ModelHierarchyAsset.h"
#include "memory/bitset.h"

class Model {
    const ModelHierarchyAsset* hierarchy{};

    Transform* localTransforms{};
    Transform* finalTransforms{};
    size_t* dirtyTransforms{};
public:
    Model() = default;

    Model(const ModelHierarchyAsset* hierarchy) : hierarchy(hierarchy) {
        localTransforms = new Transform[hierarchy->numNodes];
        finalTransforms = new Transform[hierarchy->numNodes];
        dirtyTransforms = new size_t[1 + hierarchy->numNodes / 64]{};

        std::memcpy(localTransforms, hierarchy->localNodeTransforms, sizeof(Transform) * hierarchy->numNodes);

        recalculateAllFinalTransforms();
    }

    Model(const Model&) = delete;
    Model(Model&& other) noexcept = default;
    Model& operator=(const Model&) = delete;
    Model& operator=(Model&&) = default;

    void setTransformDirty(ModelPartID part) {
        mem::bitset_view bv(dirtyTransforms, 1 + hierarchy->numNodes / 64);
        bv.set(part);
    }

    void setLocalTransform(ModelPartID part, const Transform& transform) {
        assert(hierarchy->numNodes > part);

        localTransforms[part] = transform;

        setTransformDirty(part);
    }

    void recalculateAllFinalTransforms() {
        finalTransforms[0] = localTransforms[0];

        for (size_t i = 0; i < hierarchy->numNodes; ++i) {
            auto& node = hierarchy->nodes[i];
            for (size_t j = 0; j < node.numChildren; ++j) {
                size_t childIdx = node.firstChild + j;
                finalTransforms[childIdx] = finalTransforms[i] * localTransforms[childIdx];
            }
        }
    }

    mem::range<const Transform> getLocalTransforms() const {
        return mem::make_range(localTransforms, hierarchy->numNodes);
    }

    mem::range<const Transform> getFinalTransforms() const {
        return mem::make_range(finalTransforms, hierarchy->numNodes);
    }

    mem::range<Transform> getLocalTransforms() {
        return mem::make_range(localTransforms, hierarchy->numNodes);
    }

    mem::bitset_view<size_t> getDirtyTransforms() {
        return mem::bitset_view(dirtyTransforms, 1 + hierarchy->numNodes / 64);
    }

    void calculateWorldTransforms(const glm::mat4& objTransform, glm::mat4* outWorldTransforms) const {
        for (size_t i = 0; i < hierarchy->numNodes; ++i) {
            outWorldTransforms[i] = objTransform * finalTransforms[i].createModel3D();
        }
    }

    void calculateSkinnedTransforms(const glm::mat4* inFinalTransforms, glm::mat4* outSkinnedTransforms) const {
        outSkinnedTransforms[0] = glm::mat4(1.0f);

        for (size_t i = 1; i < hierarchy->numBones; ++i) {
            auto& bone = hierarchy->bones[i];
            outSkinnedTransforms[i] = inFinalTransforms[bone.nodeID] * bone.offsetMatrix;
        }
    }

    const ModelHierarchyAsset* getAsset() const {
        return hierarchy;
    }

    const Transform& getRestPose(const ModelPartID part) const {
        return hierarchy->localNodeTransforms[part];
    }
};
