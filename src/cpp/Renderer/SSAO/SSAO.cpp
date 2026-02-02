#include "SSAO.h"
#include <random>
#include <openGL/Shader/ShaderCompiler.h>
#include <Renderer/RenderingStages/LoadPass.h>

#include "SSAOPass.h"
#include "SSAOResource.h"
#include <Renderer/Renderer.h>
#include <Renderer/Graphics/DrawContainers/StandardDrawContainer.h>
#include <Renderer/Resource/Buffer/BufferComponentType.h>
#include <Renderer/Resource/Geometry/GeometryQuery.h>
#include <Renderer/Resource/Shader/ShaderFactory.h>
#include <Renderer/Resource/Shader/ShaderQuery.h>
#include <Renderer/Resource/Texture/RenderTexture.h>
#include <Renderer/Resource/Texture/TextureFactory.h>

void createSSAOKernel(int samples, glm::vec4* outSamples) {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator{};

    for (unsigned int i = 0; i < samples; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator)
        );
        sample  = glm::normalize(sample);
        sample *= randomFloats(generator);

        float scale = static_cast<float>(i) / samples;
        scale   = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;

        outSamples[i] = glm::vec4(sample, 0);
    }
}

void createSSAONoise(int noiseSize, glm::vec3* outSSAONoise) {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator{};

    for (unsigned int i = 0; i < noiseSize; i++) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            0.0f);
        outSSAONoise[i] = noise;
    }
}

void SSAOResources::onLoad(RendererLoadView renderer) {
    ssaoShader = ShaderFactory(renderer.getRenderer()).loadShader({
        .vertex = "shaders/screen/screen_vertex.glsl",
        .fragment = "shaders/SSAO/ssao_frag.glsl"
    });

    ssaoBlurShader = ShaderFactory(renderer.getRenderer()).loadShader({
        .vertex = "shaders/screen/screen_vertex.glsl",
        .fragment = "shaders/SSAO/ssao_blur_frag.glsl"
    });
}

SSAOSettings SSAOResources::createSSAOSettings(Renderer &renderer, int kernelSize, int noiseResolution) {
    TextureCreateParams noiseParams;
    noiseParams.format = TextureFormat::RGB_32F;
    noiseParams.minFilter = TextureMinFilter::NEAREST;
    noiseParams.magFilter = TextureMagFilter::NEAREST;
    noiseParams.wrap = TextureWrap::REPEAT;

    int noiseResX = noiseResolution / 4;
    int noiseResY = noiseResolution / 4;

    Image ssaoNoiseImage = ImageLoader::procedural(noiseResX, noiseResY, PixelType::FLOAT_32, ImageChannels::RGB);

    glm::vec3* noise = renderer.getRenderAllocator()->allocate<glm::vec3>(noiseResolution);

    createSSAONoise(noiseResolution, noise);

    for (unsigned u = 0; u < noiseResX; ++u) {
        for (unsigned v = 0; v < noiseResY; ++v) {
            DynamicPixel pixel;

            glm::vec3 noiseVec = noise[u * noiseResX + v];

            std::memcpy(&pixel, &noiseVec, sizeof(glm::vec3));

            ssaoNoiseImage.store(u, v, pixel);
        }
    }

    TextureFactory tf(renderer);

    auto ssaoNoiseTexture = tf.createTexture2D(TextureDescriptor2D(noiseParams, std::move(ssaoNoiseImage)));

    auto [kernelBuffer, kernelData] = renderer.getBufferStorage().createBufferWithData<glm::vec4>(kernelSize, BufferUsageHint::IMMUTABLE);

    createSSAOKernel(kernelSize, kernelData.data());

    SSAOSettings settings;
    settings.kernelSize = kernelSize;
    settings.noiseResolution = noiseResolution;
    settings.kernelBuffer = kernelBuffer;
    settings.noiseTexture = ssaoNoiseTexture;
    return settings;
}

void SSAOPass::render(GraphicsContext &ctx) const {
    ShaderQuery sq(*ctx.getRenderer());
    GeometryQuery gq(*ctx.getRenderer());

    SSAOResources& ssaoResources = ctx.getRenderer()->getResource<SSAOResources>();

    DrawCommand cmd;
    cmd.geometry = ctx.getRenderer()->getFullScreenQuad();
    cmd.drawRange = gq.getFullDrawRange(cmd.geometry);

    {
        auto& ssaoProgram = sq.getShaderProgram(ssaoResources.getSSAOShader());

        ctx.bindShaderProgram(&ssaoProgram);

        const auto kernelSizeParameter = ssaoProgram.getUniform(ssaoProgram.getUniformParameter("kernelSize"));
        const auto depthBufferParameter = ssaoProgram.getUniform(ssaoProgram.getUniformParameter("depthBuffer"));
        const auto noiseTextureParameter = ssaoProgram.getUniform(ssaoProgram.getUniformParameter("texNoise"));
        const auto radiusParameter = ssaoProgram.getUniform(ssaoProgram.getUniformParameter("radius"));
        const auto biasParameter = ssaoProgram.getUniform(ssaoProgram.getUniformParameter("bias"));

        SamplerKey noiseSampler(desc.settings.noiseTexture);

        PushConstantSet::bind(ctx, &ssaoProgram, *kernelSizeParameter, &desc.settings.kernelSize);
        PushConstantSet::bind(ctx, &ssaoProgram, *depthBufferParameter, &desc.inDepthTexture);
        PushConstantSet::bind(ctx, &ssaoProgram, *noiseTextureParameter, &noiseSampler);
        PushConstantSet::bind(ctx, &ssaoProgram, *radiusParameter, &desc.settings.radius);
        PushConstantSet::bind(ctx, &ssaoProgram, *biasParameter, &desc.settings.bias);

        BufferBindingsSet bindings(ctx.getRenderer()->getRenderAllocator(), 2);

        bindings.add({
            .buffer = desc.cameraBuffer,
            .index = ssaoProgram.definition().getBufferBinding("Camera").second
        });
        bindings.add({
            .buffer = desc.settings.kernelBuffer,
            .index = ssaoProgram.definition().getBufferBinding("Samples").second
        });
        bindings.bind(ctx);

        ctx.bindTextureForRendering(RenderTexture(desc.outSSAONoise));
        StandardDrawContainer::draw(ctx, &cmd, 1);
    }

    {
        auto& ssaoBlurProgram = sq.getShaderProgram(ssaoResources.getSSAOBlurShader());

        ctx.bindShaderProgram(&ssaoBlurProgram);

        SamplerKey ssaoSampler(desc.outSSAONoise);

        const auto ssaoParameter = ssaoBlurProgram.getUniform(ssaoBlurProgram.getUniformParameter("ssao"));

        PushConstantSet::bind(ctx, &ssaoBlurProgram, *ssaoParameter, &ssaoSampler);

        ctx.bindTextureForRendering(RenderTexture(desc.outSSAOResult));

        StandardDrawContainer::draw(ctx, &cmd, 1);
    }

}
