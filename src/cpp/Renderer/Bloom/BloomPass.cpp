#include "BloomPass.h"

#include <Renderer/Graphics/RenderPass.h>
#include <Renderer/Graphics/DrawContainers/StandardDrawContainer.h>
#include <Renderer/Bloom/BloomRenderer.h>
#include <Renderer/Resource/Geometry/GeometryQuery.h>
#include <Renderer/Resource/Shader/ShaderQuery.h>
#include "BloomRenderer.h"

void BloomPass::render(GraphicsContext &ctx) const {
    GeometryQuery gq(*ctx.getRenderer());
    ShaderQuery sq(*ctx.getRenderer());

    auto& bloomResources = ctx.getRenderer()->getResource<BloomResourceSystem>();

    GeometryKey fullScreenGeometry = ctx.getRenderer()->getFullScreenQuad();

    DrawCommand cmd;
    cmd.geometry = fullScreenGeometry;
    cmd.drawRange = gq.getFullDrawRange(cmd.geometry);

    {
        auto& bloomExtractProgram = sq.getShaderProgram(bloomResources.bloomExtractShader);
        PushConstantSet pushSet = PushConstantSet::allocate(ctx.getRenderer()->getRenderAllocator(), &bloomExtractProgram);

        pushSet.push("scene", inBloomTexture);
        pushSet.push("threshold", bloom.threshold);

        ctx.bindTextureForRendering(RenderTexture(outBloomResult));
        ctx.bindShaderProgram(&bloomExtractProgram);
        pushSet.bind(ctx);

        StandardDrawContainer::draw(ctx, &cmd, 1);
    }

    auto& bloomDownSampleProgram = sq.getShaderProgram(bloomResources.downsampleShader);

    ctx.bindShaderProgram(&bloomDownSampleProgram);

    PushConstantSet downSamplePushSet = PushConstantSet::allocate(ctx.getRenderer()->getRenderAllocator(), &bloomDownSampleProgram);

    // assert(outBloomResult.mips >= bloom.passes)

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Downsample");
    for (int i = 0; i < bloom.passes; ++i) {
        downSamplePushSet.push("mipLevel", i);
        downSamplePushSet.push("srcTexture", outBloomResult);

        downSamplePushSet.bind(ctx);
        ctx.bindTextureForRendering(RenderTexture(outBloomResult, {}, Mipmap(i + 1)));

        StandardDrawContainer::draw(ctx, &cmd, 1);
    }
    glPopDebugGroup();
    auto& bloomUpsampleProgram = sq.getShaderProgram(bloomResources.upsampleShader);

    ctx.bindShaderProgram(&bloomUpsampleProgram);

    PushConstantSet upsamplePushSet = PushConstantSet::allocate(ctx.getRenderer()->getRenderAllocator(), &bloomUpsampleProgram);

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Upsample");
    for (int i = bloom.passes - 1; i > 0; --i) {
        RenderState state;
        state.blendState.enabled = true;
        state.blendState.rgbEquation = BlendEquation::FuncAdd;
        state.blendState.srcRGB = BlendFunction::One;
        state.blendState.dstRGB = BlendFunction::One;
        state.blendState.alphaEquation = BlendEquation::FuncAdd;
        state.blendState.srcAlpha = BlendFunction::One;
        state.blendState.dstAlpha = BlendFunction::One;

        upsamplePushSet.push("srcTexture", outBloomResult);
        upsamplePushSet.push("filterRadius", bloom.filterRadius);
        upsamplePushSet.push("mipLevel", i);

        upsamplePushSet.bind(ctx);
        ctx.bindTextureForRendering(RenderTexture(outBloomResult, {}, Mipmap(i - 1)));
        ctx.setCurrentState(state);

        StandardDrawContainer::draw(ctx, &cmd, 1);
    }
    glPopDebugGroup();
}
