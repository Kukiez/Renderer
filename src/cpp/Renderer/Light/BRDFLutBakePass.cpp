#include "BRDFLutBakePass.h"
#include <Renderer/Graphics/DrawContainers/StandardDrawContainer.h>
#include <Renderer/Resource/Geometry/GeometryQuery.h>
#include <Renderer/Resource/Shader/ShaderQuery.h>

#include "LightSystem.h"

void BRDFLutBakePass::render(GraphicsContext &ctx) const {
    GeometryQuery geoQ(*ctx.getRenderer());
    ShaderQuery sq(*ctx.getRenderer());

    auto shaderKey = ctx.getRenderer()->getResource<LightSystem>().getBRDFLutShader();

    ctx.bindShaderProgram(&sq.getShaderProgram(shaderKey));

    ctx.bindTextureForRendering(RenderTexture(brdfLUT));

    DrawCommand cmd;
    cmd.geometry = ctx.getRenderer()->getFullScreenQuad();
    cmd.drawRange = geoQ.getFullDrawRange(cmd.geometry);
    StandardDrawContainer::draw(ctx, &cmd, 1);
}
