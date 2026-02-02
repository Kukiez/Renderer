#pragma once
#include "ModelResource.h"
#include "Renderer/Scene/Primitives/IPrimitive.h"
#include "Math/Shapes/AABB.h"
#include "Renderer/RenderingStages/OpaquePass.h"
#include "Renderer/Scene/Primitives/PrimitiveArray.h"
#include "Renderer/Scene/Primitives/PrimitiveWorld.h"

struct MeshPrimitive {
    unsigned meshSection{};
};

struct MeshPrimitiveArray : IPrimitiveArray<MeshPrimitive> {
    ModelKey model{};
};

using TConstMeshPrimitiveArray = TConstPrimitiveArray<MeshPrimitiveArray>;

struct SkinnedMeshCollection : ITPrimitiveCollection<MeshPrimitiveArray> {
    unsigned baseTransformIdx{};

    SkinnedMeshCollection() = default;

    explicit SkinnedMeshCollection(unsigned baseTransformIdx) : baseTransformIdx(baseTransformIdx) {}
};

struct StaticMeshCollection : ITPrimitiveCollection<MeshPrimitiveArray> {
    unsigned baseMeshIdx{};

    StaticMeshCollection() = default;
    StaticMeshCollection(unsigned baseMeshIdx) : baseMeshIdx(baseMeshIdx) {}
};

namespace mesh {
    inline TConstMeshPrimitiveArray createPrimitiveArray(PrimitiveWorld& world, const ModelAsset &asset) {
        size_t numPrimitivesReq = 0;

        for (auto& meshIndices : asset.getMeshIndices()) {
            numPrimitivesReq += meshIndices.numIndices;
        }

        auto& array = world.createPrimitiveArray<MeshPrimitiveArray>(numPrimitivesReq);

        unsigned nextPrimIdx = 0;
        for (auto& meshIndices : asset.getMeshIndices()) {
            for (auto mesh : meshIndices) {
                auto primitive = array[nextPrimIdx];

                primitive->meshSection = mesh;
                primitive.setLocalBounds(AABB::fromTo(glm::vec3(1), glm::vec3(16)));
                primitive.getPasses().set<OpaqueRenderingPass>();
                ++nextPrimIdx;
            }
        }
        array->model = asset.getKey();
        std::cout << "Primitives required: " << numPrimitivesReq << std::endl;
        return array;
    }
}