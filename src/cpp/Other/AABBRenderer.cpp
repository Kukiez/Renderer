#pragma once

#include "AABBRenderer.h"
#include <Util/Random.h>

#include "../Renderer/Graphics/RenderPass.h"
#include "../Renderer/Graphics/DrawContainers/StandardDrawContainer.h"
#include "../Renderer/RenderingStages/LoadPass.h"
#include "../Renderer/RenderingStages/OpaquePass.h"
#include "../Renderer/Pipeline/PassInvocation.h"
#include "../Renderer/Resource/Geometry/GeometryBuilder.h"
#include "../Renderer/Resource/Geometry/GeometryQuery.h"
#include "../Renderer/Resource/Shader/ShaderFactory.h"

void AABBRenderer::onLoad(RendererLoadView level) {
    ShaderFactory shaderFactory(level.getRenderer());
    GeometryFactory geometryFactory(level.getRenderer());

    glm::vec3 v0(-1, -1, -1);
    glm::vec3 v1( 1, -1, -1);
    glm::vec3 v2( 1,  1, -1);
    glm::vec3 v3(-1,  1, -1);
    glm::vec3 v4(-1, -1,  1);
    glm::vec3 v5( 1, -1,  1);
    glm::vec3 v6( 1,  1,  1);
    glm::vec3 v7(-1,  1,  1);

    std::array unitWireframeVertices = {
        v0, v1, v1, v5, v5, v4, v4, v0,
        v3, v2, v2, v6, v6, v7, v7, v3,
        v0, v3, v1, v2, v5, v6, v4, v7
    };

    aabbGeometry = geometryFactory.loadGeometry(GeometryDescriptor(unitWireframeVertices, VertexLayoutOf(VertexAttribute::Position3)));

    aabbShader = shaderFactory.loadShader({
        .vertex = "shaders/Debug/AABB/aabb_vertex.glsl",
        .fragment = "shaders/Debug/AABB/aabb_frag.glsl"
    });
    aabbTransparentShader = shaderFactory.loadShader({
        .vertex = "shaders/Debug/AABB/aabb_vertex.glsl",
        .fragment = "shaders/Debug/AABB/aabb_transparent_frag.glsl"
    });
}

void SimpleShapePipeline::onRender(const GraphicsPassInvocation &invocation) const {
    auto& renderer = invocation.getRenderer();
    auto boxes = invocation.getVisiblePrimitives().getVisible<BoxCollection>();

    if (boxes.empty()) return;

    auto& renderPass = invocation.createRenderPass();
    auto& pass = renderPass.createPass<GraphicsPass>("SimpleShapePass", shader);

    pass.bind(invocation.getViewBuffer(), "Camera");

    GeometryQuery geoQ(renderer);

    auto [opaqueBuffer, opaqueBoxes] = renderer.getBufferStorage().createBufferWithData<DrawAABBCommand>(boxes.size(), BufferUsageHint::FRAME_SCRATCH_BUFFER);
    pass.bind(opaqueBuffer, "Instances");

    size_t next = 0;
    for (auto prim : boxes) {
        opaqueBoxes[next] = DrawAABBCommand::fromAABB(prim.getCollection()->aabb, prim.getCollection()->color);
        ++next;
    }

    auto& drawContainer = pass.usingDrawContainer<StandardDrawContainer>(boxes.size());
    DrawCommand cmd;
    cmd.geometry = geometry;
    cmd.drawRange = geoQ.getFullDrawRange(cmd.geometry);
    cmd.drawRange.firstInstance = 0;
    cmd.drawRange.instanceCount = boxes.size();
    cmd.drawPrimitive = DrawPrimitive::LINES;
    drawContainer.draw(cmd);
}
