
#include "SkyboxPass.h"

#include <Renderer/Graphics/GraphicsContext.h>
#include <Renderer/Graphics/PassBindings.h>
#include <Renderer/Graphics/PushConstant.h>
#include <Renderer/Graphics/DrawContainers/StandardDrawContainer.h>
#include <Renderer/Resource/Geometry/GeometryQuery.h>
#include <Renderer/Resource/Shader/ShaderQuery.h>

void SkyboxPass::render(GraphicsContext &ctx) const {
    GeometryQuery geoQ(*ctx.getRenderer());
    ShaderQuery sq(*ctx.getRenderer());

    auto& skyboxResource = ctx.getRenderer()->getSystem<SkyboxRenderer>();

    auto& program = sq.getShaderProgram(skyboxResource.skyboxShader);

    BufferBindingsSet pass(ctx.getRenderer()->getRenderAllocator(), 3);
    PushConstantSet pushes(ctx.getRenderer()->getRenderAllocator(), &program);

    IGraphicsPass::bind(pass, &program, view, "Camera");
    pushes.push("skybox", skybox);

    RenderState state{};
    state.cullMode = CullMode::None;
    state.depthState.enabled = true;
    state.depthState.func = DepthFunction::Lequal;

    DrawCommand cmd;
    cmd.geometry = skyboxResource.cubemapGeometryKey;
    cmd.drawRange = geoQ.getFullDrawRange(cmd.geometry);

    ctx.setCurrentState(state);
    ctx.bindTextureForRendering(MultiRenderTexture(
        colorTarget, depthTarget
    ));
    ctx.bindShaderProgram(&program);

    pushes.bind(ctx);
    pass.bind(ctx);

    StandardDrawContainer::draw(ctx, &cmd, 1);
}
