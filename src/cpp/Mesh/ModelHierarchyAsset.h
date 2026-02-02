#pragma once
#include <Renderer/Common/Transform.h>

enum ModelPartID : unsigned {
    INVALID = std::numeric_limits<unsigned>::max()
};

struct ModelHierarchyAsset {
    struct Node {
        std::string_view name{};
        int parentID{};
        int firstChild{};
        int numChildren{};
    };

    struct Bone {
        int nodeID{};
        glm::mat4 offsetMatrix{};
    };

    Node* nodes{};
    Transform* localNodeTransforms{};
    size_t numNodes{};

    /* First bone is fake (mat4(1.0f)) */
    Bone* bones{};
    size_t numBones{};

    ModelAnimation* animations{};
    size_t numAnimations{};

    int* nodeToBone{};

    ModelHierarchyAsset() = default;
    ModelHierarchyAsset(const ModelCooking::ModelLoadingCache& cache) {
        nodes = new Node[cache.numNodes];
        localNodeTransforms = new Transform[cache.numNodes];
        numNodes = cache.numNodes;

        for (size_t i = 0; i < cache.numNodes; ++i) {
            auto& node = cache.hierarchy[i];

            nodes[i] = {
                cache.hierarchyNodeNames[i],
                {},
                node.firstChild,
                node.numChildren
            };

            localNodeTransforms[i] = cache.hierarchyLocalTransforms[i];
        }

        if (cache.boneCounter > 1) {
            bones = new Bone[cache.boneCounter];
            numBones = cache.boneCounter;

            bones[0].nodeID = 0;
            for (size_t i = 1; i < cache.boneCounter; ++i) {
                auto& bone = cache.bones[i];

                bones[i].nodeID = bone.nodeID;
                bones[i].offsetMatrix = bone.offsetMatrix;
            }

            nodeToBone = new int[cache.numNodes];

            for (size_t i = 0; i < cache.numNodes; ++i) {
                nodeToBone[i] = cache.nodeToBoneIndex[i];
            }
        }
    }

    mem::range<const Node> getNodes() const {
        return mem::make_range(nodes, numNodes);
    }

    mem::range<const Node> getChildren(const Node& node) const {
        return mem::make_range(&nodes[node.firstChild], node.numChildren);
    }

    mem::range<const Bone> getBones() const {
        return mem::make_range(bones, numBones);
    }

    size_t getNodeIndex(const Node* node) const {
        assert(node >= nodes && node < nodes + numNodes);
        return node - nodes;
    }

    size_t getNodeIndex(const Node& node) const {
        return getNodeIndex(&node);
    }

    size_t getNodesCount() const { return numNodes; }
    size_t getBonesCount() const { return numBones; }
    size_t getAnimationsCount() const { return numAnimations; }
};