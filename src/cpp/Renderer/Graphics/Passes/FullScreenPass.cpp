#include "FullScreenPass.h"

#include "openGL/Shader/ShaderCompiler.h"
#include "Renderer/Graphics/DrawContainers/StandardDrawContainer.h"
#include "Renderer/Resource/Geometry/GeometryQuery.h"
#include "Renderer/Resource/Shader/ShaderQuery.h"

void FullScreenPass::render(GraphicsContext &ctx) const {
    auto fullScreenQuad = ctx.getRenderer()->getFullScreenQuad();
    auto fullScreenShader = ctx.getRenderer()->getFullScreenShader();

    ShaderQuery sq(*ctx.getRenderer());
    GeometryQuery gq(*ctx.getRenderer());

    auto& program = sq.getShaderProgram(fullScreenShader);
    ctx.bindShaderProgram(&program);

    auto sceneParameter = program.getUniformParameter("scene");

    PushConstantSet::bind(ctx, &program, sceneParameter, &inTexture);

    ctx.bindTextureForRendering(outTexture);

    RenderState state;
    state.viewport = vp;

    ctx.setCurrentState(state);

    DrawCommand cmd;
    cmd.geometry = fullScreenQuad;
    cmd.drawRange = gq.getFullDrawRange(cmd.geometry);

    StandardDrawContainer::draw(ctx, &cmd, 1);
}
