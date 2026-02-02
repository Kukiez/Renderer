#include "SkyboxBakePass.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <Renderer/Graphics/RenderPass.h>
#include <Renderer/Graphics/Passes/EnvCubemapPass.h>
#include <Renderer/Graphics/Passes/PrefilterPass.h>
#include <Renderer/Graphics/UtilityPasses/GenerateMipmapPass.h>
#include <Renderer/Resource/Texture/TextureFactory.h>
#include <Renderer/Resource/Texture/TextureKey.h>
#include <Renderer/Skybox/SkyboxSystem.h>
#include <Util/enum_bit.h>

struct SkyboxBakingContext {
    Renderer& renderer;
    ShaderKey hdriLoadShader;
    ShaderKey irradianceMapShader;
    ShaderKey prefilterMapShader;
    std::string_view hdriFile;

    RenderPass renderPass;

    Skybox* outSkybox{};
};

static auto SkyboxCaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

TextureKey createHDRITexture(const SkyboxBakingContext& ctx) {
    TextureFactory texFactory(ctx.renderer);

    Image image = ImageLoader::load(ctx.hdriFile.data(), ImageLoadOptions::FLIP_VERTICALLY);

    TextureCreateParams hdriTextureParams;
    hdriTextureParams.minFilter = TextureMinFilter::LINEAR;
    hdriTextureParams.magFilter = TextureMagFilter::LINEAR;
    hdriTextureParams.wrap = TextureWrap::EDGE_CLAMP;
    hdriTextureParams.format = TextureFormat::RGBA_16F;

    const TextureKey hdriTextureKey = texFactory.createTexture2D(TextureDescriptor2D(hdriTextureParams, std::move(image)));
    // TODO destroy the texture
    return hdriTextureKey;
}

void renderEnvCubemap(SkyboxBakingContext& ctx) {
    TextureFactory texFactory(ctx.renderer);

    TextureKey hdriTexture = createHDRITexture(ctx);

    TextureCreateParams envCubemapParams;
    envCubemapParams.format = TextureFormat::RGB_16F;
    envCubemapParams.wrap = TextureWrap::EDGE_CLAMP;
    envCubemapParams.mipmap = Mipmap::AUTO;

    const TextureKey envCubemapTexture = texFactory.createCubemap(envCubemapParams, 512, 512); // 512 -> in shader aswell

    auto& envCubemapPass = ctx.renderPass.createPass<EnvCubemapPass>("SkyboxEnvPass", ctx.hdriLoadShader);
    envCubemapPass.setClearTargets(ClearTarget::COLOR | ClearTarget::DEPTH);
    envCubemapPass.setCaptureProjection("projection", SkyboxCaptureProjection);
    envCubemapPass.setInputTexture("hdri", hdriTexture);
    envCubemapPass.setViews("view", Skybox::HDRIViews.data());
    envCubemapPass.setTargetCubemap(envCubemapTexture);

    ctx.renderPass.createPass<GenerateMipmapPass>("GenerateMipmapPass", envCubemapTexture);

    ctx.outSkybox->envCubemap = envCubemapTexture;
}

void renderIrradianceMap(SkyboxBakingContext& ctx) {
    TextureFactory texFactory(ctx.renderer);

    TextureCreateParams irradianceMapParams;
    irradianceMapParams.format = TextureFormat::RGB_16F;
    irradianceMapParams.wrap = TextureWrap::EDGE_CLAMP;
    irradianceMapParams.mipmap = Mipmap::NONE;

    const TextureKey irradianceMapTexture = texFactory.createCubemap(EmptyCubemapDescriptor2D(irradianceMapParams, 32, 32));

    auto& irradiancePass = ctx.renderPass.createPass<EnvCubemapPass>("SkyboxIrradiancePass", ctx.irradianceMapShader);
    irradiancePass.setClearTargets(ClearTarget::COLOR | ClearTarget::DEPTH);
    irradiancePass.setTargetCubemap(irradianceMapTexture);
    irradiancePass.setInputTexture("envCubemap", ctx.outSkybox->envCubemap);
    irradiancePass.setViews("view", Skybox::HDRIViews.data());
    irradiancePass.setCaptureProjection("projection", SkyboxCaptureProjection);

    ctx.outSkybox->irradianceMap = irradianceMapTexture;
}

void renderPrefilterMap(SkyboxBakingContext& ctx) {
    TextureFactory texFactory(ctx.renderer);

    TextureCreateParams prefilterMapParams;
    prefilterMapParams.format = TextureFormat::RGB_16F;
    prefilterMapParams.wrap = TextureWrap::EDGE_CLAMP;
    prefilterMapParams.mipmap = Mipmap(5);
    prefilterMapParams.minFilter = TextureMinFilter::LINEAR_MIPMAP_LINEAR;
    prefilterMapParams.magFilter = TextureMagFilter::LINEAR;

    EmptyCubemapDescriptor2D prefilterDescriptor;
    prefilterDescriptor.width = 256;
    prefilterDescriptor.height = 256;
    prefilterDescriptor.params = prefilterMapParams;

    TextureKey prefilterMapTexture = texFactory.createCubemap(prefilterDescriptor);

    const int mipLevels = 5;
    auto& prefilterPass = ctx.renderPass.createPass<PrefilterMapPass>("SkyboxPrefilterPass", ctx.prefilterMapShader);

    prefilterPass.setRoughness("roughness");
    prefilterPass.setClearTargets(ClearTarget::COLOR | ClearTarget::DEPTH);
    prefilterPass.setTargetCubemap(prefilterMapTexture);
    prefilterPass.setInputTexture("envCubemap", ctx.outSkybox->envCubemap);
    prefilterPass.setCaptureProjection("projection", SkyboxCaptureProjection);
    prefilterPass.setViews("view", Skybox::HDRIViews.data());
    prefilterPass.setMips(Mipmap{static_cast<uint8_t>(mipLevels)});

    ctx.outSkybox->prefilterMap = prefilterMapTexture;
    ctx.outSkybox->prefilterLOD = mipLevels;
}

void SkyboxBakePass::render(GraphicsContext &ctx) const {
    auto& skyboxResources = ctx.getRenderer()->getResource<SkyboxRenderer>();
    
    SkyboxBakingContext bakingContext(
        *ctx.getRenderer(),
        skyboxResources.hdriLoadShader,
        skyboxResources.irradianceMapShader,
        skyboxResources.prefilterMapShader,
        hdriFile,
        RenderPass(ctx.getRenderer()),
        outSkybox
    );
    renderEnvCubemap(bakingContext);
    renderIrradianceMap(bakingContext);
    renderPrefilterMap(bakingContext);

    ctx.getRenderer()->synchronize();
    ctx.getRenderer()->render(bakingContext.renderPass);
}
