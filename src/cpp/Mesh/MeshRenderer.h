#pragma once

#include "MeshPrimitive.h"
#include "Model.h"
#include "ModelResource.h"
#include "Renderer/Graphics/DrawContainers/StandardDrawContainer.h"
#include "Renderer/Resource/Shader/ShaderFactory.h"
#include "Renderer/Resource/Texture/TextureQuery.h"

struct GPUStaticMeshInstance {
    int transformIdx;
    int materialIdx;
    int staticMaterialIdx;
};

struct MeshRenderer {
    BufferKey meshTransformBuffer;

    ShaderKey depthShader;
    ShaderKey meshShader;

    mem::piece_list freeSpots;

    void onLoad(RendererLoadView view) {
        ShaderFactory sf(view.getRenderer());

        depthShader = sf.loadShader({
            .vertex = "shaders/Mesh/mesh_geometry.glsl",
            .fragment = "shaders/common/empty.glsl"
        });
        meshShader = sf.loadShader({
            .vertex = "shaders/Mesh/mesh_geometry.glsl",
            .fragment = "shaders/Mesh/mesh_fragment.glsl"
        });
        meshTransformBuffer = view.getRenderer().getBufferStorage().createBuffer<glm::mat4>(1024, BufferUsageHint::PERSISTENT_WRITE_ONLY);
        freeSpots.free({0, 1024});
    }

    StaticMeshCollection createStaticMesh(const Transform& worldTransform, const Model& model) {
        mem::piece_list::piece piece = freeSpots.find(model.getFinalTransforms().size());

        auto transformData = meshTransformBuffer.mapRange<glm::mat4>(piece.begin, piece.count);
        model.calculateWorldTransforms(worldTransform.createModel3D(), transformData.data());

        return StaticMeshCollection(piece.begin);
    }
};

class StaticMeshPipeline : public IPrimitivePipeline<StaticMeshCollection> {
public:
    void onRender(const GraphicsPassInvocation& invocation) const {
        auto visiblePrimitives = invocation.getVisiblePrimitives().getVisible<StaticMeshCollection>();

        if (visiblePrimitives.empty()) return;

        TextureQuery tq(invocation.getRenderer());

        auto& skinnedMeshes = invocation.getRenderer().getSystem<MeshRenderer>();
        auto& modelResources = invocation.getRenderer().getResource<ModelResourceSystem>();

        auto [instancesBuffer, instances] = invocation.getRenderer().getBufferStorage().createBufferWithData<GPUStaticMeshInstance>(visiblePrimitives.size(), BufferUsageHint::FRAME_SCRATCH_BUFFER);

        RenderPass& renderPass = invocation.createRenderPass();

        auto& pass = renderPass.createPass<GraphicsPass>("MeshOpaquePass", skinnedMeshes.meshShader);

        pass.bind(invocation.getViewBuffer(), "Camera");
        pass.bind(skinnedMeshes.meshTransformBuffer, "MeshTransforms");
        pass.bind(tq.getMaterial2DBuffer(), "MaterialData");
        pass.bind(modelResources.getPBRMaterialsBuffer(), "MeshMaterialHeader");
        pass.bind(instancesBuffer, "MeshInstances");

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

            instances[cmdIdx].transformIdx = collection->baseMeshIdx;
            instances[cmdIdx].staticMaterialIdx = mesh.materialIndex;
            ++cmdIdx;
        }
    }
};