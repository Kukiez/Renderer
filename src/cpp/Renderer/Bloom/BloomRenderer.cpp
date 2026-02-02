#include "BloomRenderer.h"

#include <Renderer/Resource/Shader/ShaderFactory.h>

#include "BloomPass.h"

void BloomResourceSystem::onLoad(RendererLoadView renderer) {
    ShaderFactory shaderFactory(renderer.getRenderer());

    upsampleShader = shaderFactory.loadShader({
        .vertex = "shaders/screen/screen_vertex.glsl",
        .fragment = "shaders/bloom/bloom_upsample.glsl"
    });
    downsampleShader = shaderFactory.loadShader({"shaders/screen/screen_vertex.glsl", "shaders/bloom/bloom_downsample.glsl"});
    bloomExtractShader = shaderFactory.loadShader({"shaders/screen/screen_vertex.glsl", "shaders/bloom/bloom_extract.glsl"});
    applyBloomShader = shaderFactory.loadShader({"shaders/screen/screen_vertex.glsl", "shaders/bloom/bloom_apply.glsl"});
}