#pragma once
#include "Model.h"
#include "ModelResource.h"
#include "MeshPrimitive.h"

struct GPUSkinnedMeshInstance {
    int boneIndex;
    int materialIndex;
    int staticMaterialIndex;
};

class SkinnedMeshRenderer {
    friend class SkinnedMeshPipeline;

    ShaderKey meshShader{};
    BufferKey matricesBuffer{};

    ModelResourceSystem* modelType{};

    mem::piece_list freeSpots;

    std::vector<mem::piece_list::piece> freePieces[3];
    int frame = 0;
public:
    void onLoad(RendererLoadViewExt view) {
        ShaderFactory sf(view.getRenderer());

        meshShader = sf.loadShader({
            .vertex = "shaders/materials/Mesh/mesh_skinned_geometry.glsl",
            .fragment = "shaders/materials/Mesh/mesh_fragment.glsl"
        });

        auto& bf = view.getRenderer().getBufferStorage();
        matricesBuffer = bf.createBuffer<glm::mat4>(100, BufferUsageHint::PERSISTENT_WRITE_ONLY);
        freeSpots = mem::piece_list(100);

        auto& models = view.getResource<ModelResourceSystem>();

        modelType = &models;
    }

    void writeSkinnedMeshData(Renderer& renderer, mem::piece_list::piece piece, const Transform& worldTransform, const Model& model) {
        auto transformData = matricesBuffer.mapRange<glm::mat4>(piece.begin, piece.count);

        auto finalTransforms = renderer.getRenderAllocator()->allocate<glm::mat4>(model.getFinalTransforms().size());

        model.calculateWorldTransforms(worldTransform.createModel3D(), finalTransforms);
        model.calculateSkinnedTransforms(finalTransforms, transformData.data());
    }

    SkinnedMeshCollection createSkinnedMesh(Renderer& renderer, const Transform& worldTransform, const Model& model) {
        auto piece = freeSpots.find(model.getAsset()->getBonesCount());

        if (piece.count == 0) {
            const size_t oldSize = matricesBuffer.size();

            const size_t newCapacity = std::max(oldSize * 2, model.getAsset()->getBonesCount() * 2);
            matricesBuffer.resize(0, 0, matricesBuffer.size(), newCapacity);
            freeSpots.free({
                oldSize / sizeof(glm::mat4),
                newCapacity / sizeof(glm::mat4) - oldSize
            });

            piece = freeSpots.find(model.getAsset()->getBonesCount());
        }

        writeSkinnedMeshData(renderer, piece, worldTransform, model);
        return SkinnedMeshCollection(piece.begin);
    }

    void updateSkinnedMesh(Renderer& renderer, SkinnedMeshCollection& collection, const Transform& worldTransform, const Model& model) {
        auto oldTransformIdx = collection.baseTransformIdx;

        auto piece = freeSpots.find(model.getAsset()->getBonesCount());
        collection.baseTransformIdx = piece.begin;

        writeSkinnedMeshData(renderer, piece, worldTransform, model);

        freeSpots.free({
            oldTransformIdx,
            model.getAsset()->getBonesCount()
        });
    }
};

class SkinnedMeshPipeline : public IPrimitivePipeline<SkinnedMeshCollection> {
public:
    void onRender(const GraphicsPassInvocation& invocation) const {
        auto visiblePrimitives = invocation.getVisiblePrimitives().getVisible<SkinnedMeshCollection>();

        if (visiblePrimitives.empty()) return;

        TextureQuery tq(invocation.getRenderer());

        auto& skinnedMeshes = invocation.getRenderer().getSystem<SkinnedMeshRenderer>();
        auto& modelResources = invocation.getRenderer().getResource<ModelResourceSystem>();

        auto [instancesBuffer, instances] = invocation.getRenderer().getBufferStorage().createBufferWithData<GPUSkinnedMeshInstance>(visiblePrimitives.size(), BufferUsageHint::FRAME_SCRATCH_BUFFER);

        RenderPass& renderPass = invocation.createRenderPass();

        auto& pass = renderPass.createPass<GraphicsPass>("MeshOpaquePass", skinnedMeshes.meshShader);

        pass.bind(invocation.getViewBuffer(), "Camera");
        pass.bind(skinnedMeshes.matricesBuffer, "BoneTransforms");
        pass.bind(tq.getMaterial2DBuffer(), "MaterialData");
        pass.bind(skinnedMeshes.modelType->getPBRMaterialsBuffer(), "MeshMaterialHeader");
        pass.bind(instancesBuffer, "SkinnedMeshInstances");

        auto& drawContainer = pass.usingDrawContainer<StandardDrawContainer>(visiblePrimitives.size());

        unsigned cmdIdx = 0;
        for (auto prim : visiblePrimitives) {
            auto& collection = prim.getCollection();
            auto& array = prim.getArray();
            auto asset = modelResources.getAsset(array->model);

            auto sections = asset.getMeshSections();

            auto& mesh = sections[prim->meshSection];

            DrawCommand cmd;
            cmd.geometry = asset.getGeometry();
            cmd.drawRange.firstIndex = mesh.indexOffset;
            cmd.drawRange.indexCount = mesh.indexCount;
            cmd.drawRange.baseVertex = mesh.baseVertex;

            cmd.drawRange.instanceCount = 1;
            cmd.drawRange.firstInstance = cmdIdx;

            drawContainer.draw(cmd);

            instances[cmdIdx].boneIndex = collection->baseTransformIdx;
            instances[cmdIdx].staticMaterialIndex = mesh.materialIndex;

            ++cmdIdx;
        }
    }
};