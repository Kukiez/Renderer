#include "DisplayPass.h"

#include <openGL/Shader/ShaderCompiler.h>
#include <Renderer/Graphics/DrawContainers/StandardDrawContainer.h>
#include <Renderer/Resource/Geometry/GeometryQuery.h>
#include <Renderer/Resource/Shader/ShaderQuery.h>
#include <Renderer/Resource/Texture/RenderTexture.h>
#include "DisplayResources.h"

void DisplayPass::render(GraphicsContext &ctx) const {
    ctx.bindTextureForRendering(RenderTexture::DefaultScreen());

    ShaderQuery sq(*ctx.getRenderer());
    GeometryQuery gq(*ctx.getRenderer());

    ctx.setCurrentState({});

    auto& displayShaders = ctx.getRenderer()->getResource<DisplayResources>();

    auto& program = sq.getShaderProgram(displayShaders.defaultDisplayShader);

    ctx.bindShaderProgram(&program);

    auto fullScreenGeometry = ctx.getRenderer()->getFullScreenQuad();

    DrawCommand cmd;
    cmd.geometry = fullScreenGeometry;
    cmd.drawRange = gq.getFullDrawRange(cmd.geometry);

    auto sceneParameter = program.getUniform(program.getUniformParameter("scene"));

    if (desc.isBloomEnabled) {
        auto bloomParameter = program.getUniform(program.getUniformParameter("bloom"));
        auto bloomStrengthParameter = program.getUniform(program.getUniformParameter("bloomStrength"));
        auto isBloomEnabledParameter = program.getUniform(program.getUniformParameter("isBloomEnabled"));

        PushConstantSet::bind(ctx, &program, *bloomParameter, &desc.bloom);
        PushConstantSet::bind(ctx, &program, *bloomStrengthParameter, &desc.bloomStrength);
        PushConstantSet::bind(ctx, &program, *isBloomEnabledParameter, &desc.isBloomEnabled);
    }

    if (desc.tonemap.isEnabled) {
        auto exposureParameter = program.getUniform(program.getUniformParameter("tonemapExposure"));
        auto isEnabledParameter = program.getUniform(program.getUniformParameter("isTonemapEnabled"));
        auto tonemapMethodParameter = program.getUniform(program.getUniformParameter("tonemapMethod"));

        PushConstantSet::bind(ctx, &program, *exposureParameter, &desc.tonemap.exposure);
        PushConstantSet::bind(ctx, &program, *tonemapMethodParameter, &desc.tonemap.method);
        PushConstantSet::bind(ctx, &program, *isEnabledParameter, &desc.tonemap.isEnabled);
    }

    if (desc.ssao.isEnabled) {
        auto ssaoParameter = program.getUniform(program.getUniformParameter("ssao"));
        auto isEnabledParameter = program.getUniform(program.getUniformParameter("isSSAOEnabled"));

        PushConstantSet::bind(ctx, &program, *ssaoParameter, &desc.ssao.ssao);
        PushConstantSet::bind(ctx, &program, *isEnabledParameter, &desc.ssao.isEnabled);
    }
    PushConstantSet::bind(ctx, &program, *sceneParameter, &desc.scene);

    StandardDrawContainer::draw(ctx, &cmd, 1);
}

void DisplayDepthPass::render(GraphicsContext &ctx) const {
    ShaderQuery sq(*ctx.getRenderer());
    GeometryQuery gq(*ctx.getRenderer());

    auto& displayResources = ctx.getRenderer()->getResource<DisplayResources>();
    auto& program = sq.getShaderProgram(displayResources.depthDisplayShader);
    ctx.bindShaderProgram(&program);

    auto fullScreenGeometry = ctx.getRenderer()->getFullScreenQuad();

    DrawCommand cmd;
    cmd.geometry = fullScreenGeometry;
    cmd.drawRange = gq.getFullDrawRange(cmd.geometry);

    auto depthParameter = program.getUniform(program.getUniformParameter("depthTex"));

    PushConstantSet::bind(ctx, &program, *depthParameter, &depthTexture);

    BufferBindingsSet set(ctx.getRenderer()->getRenderAllocator(), 1);
    set.add({
        .buffer = cameraBuffer,
        .index = program.definition().getBufferBinding("Camera").second
    });

    set.bind(ctx);

    ctx.bindTextureForRendering(RenderTexture::DefaultScreen());

    if (viewport.enabled) {
        RenderState state;
        state.viewport = viewport;
        ctx.setCurrentState(state);
    }
    StandardDrawContainer::draw(ctx, &cmd, 1);
}
