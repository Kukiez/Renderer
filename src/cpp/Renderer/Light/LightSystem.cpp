#include "LightSystem.h"
#include <Other/AABBRenderer.h>
#include <Renderer/Graphics/Features/RenderTargetSpecific.h>
#include <Renderer/Pipeline/PassInvocation.h>
#include <Renderer/Resource/Shader/ShaderFactory.h>
#include <Renderer/Resource/Texture/TextureFactory.h>
#include <util/Random.h>
#include "LightCollection.h"
#include "BRDFLutBakePass.h"

TextureKey initializeBRDFLutBuffer(RendererLoadView view, ShaderKey shader) {
    TextureFactory texFactory(view.getRenderer());

    TextureCreateParams brdfLUTParams;
    brdfLUTParams.minFilter = TextureMinFilter::LINEAR;
    brdfLUTParams.magFilter = TextureMagFilter::LINEAR;
    brdfLUTParams.wrap = TextureWrap::EDGE_CLAMP;
    brdfLUTParams.format = TextureFormat::RG_16F;

    TextureKey brdfLUT = texFactory.createTexture2D(TextureDescriptor2D(brdfLUTParams, 512, 512));
    return brdfLUT;
}

TextureKey createShadowNoiseTex(TextureFactory tf) {
    std::mt19937 rng(0);
    std::uniform_real_distribution rand01(0.0f, 1.0f);

    static constexpr auto TextureSize = 64;

    std::array<glm::vec2, TextureSize * TextureSize> noise{};

    for (int y = 0; y < TextureSize; y++)
        for (int x = 0; x < TextureSize; x++) {
            float a = rand01(rng) * 2.0f * 3.14f;
            noise[x * TextureSize + y].x = cos(a) * 0.5f + 0.5f;
            noise[x * TextureSize +y].y = sin(a) * 0.5f + 0.5f;
        }

    TextureCreateParams noiseTexParams;
    noiseTexParams.format = TextureFormat::RG_16F;
    noiseTexParams.minFilter = TextureMinFilter::NEAREST;
    noiseTexParams.magFilter = TextureMagFilter::NEAREST;
    noiseTexParams.wrap = TextureWrap::REPEAT;

    auto noiseTexData = ImageLoader::procedural(TextureSize, TextureSize, PixelType::FLOAT_16, ImageChannels::RG);
    ImageLoader::fill(noiseTexData, noise.data(), PixelType::FLOAT_32);

    TextureKey noiseTex = tf.createTexture2D(TextureDescriptor2D(noiseTexParams, std::move(noiseTexData)));
    return noiseTex;
}

static float ClipmapSizes[] = {
    256, 640, 1280, 5120
};

static int ClipmapResolutions[] = {
    4096,
    1024,
    1024,
    1024
};

static float ClipmapShadowSoftnessScale[] = {
    32,
    2,
    32,
    32
};

float cascadeViewOffset(int cascade) {
    float dist = 0.f;

    for (int i = 0; i < cascade; ++i) {
        dist += ClipmapSizes[i];
    }
    return dist;
}

void LightSystem::onLoad(RendererLoadView view) {
    ShaderFactory shaderFactory(view.getRenderer());

    brdfLUTShader = shaderFactory.loadShader({
        .vertex = "shaders/screen/screen_vertex.glsl",
        .fragment = "shaders/Lighting/ibl_brdf_lut.glsl"
    });
    brdfLUT = initializeBRDFLutBuffer(view, brdfLUTShader);

    shadowJitterTexture = createShadowNoiseTex(TextureFactory(view.getRenderer()));

    lightClusterPartition.initialize(view.getRenderer());

    renderer = &view.getRenderer();

    RenderPass brdfLutBakePass(renderer);
    brdfLutBakePass.createPass<BRDFLutBakePass>("BRDFLutBakePass", brdfLUT);

    renderer->synchronize();
    renderer->render(brdfLutBakePass);
}

struct CascadeView {
    GPUCamera camera;
    float worldTexelSize;
    float worldFilterRadiusUV;
    float constantBias;
    float slopeBias;
};

std::vector<glm::vec3> getFrustumCornersWorldSpace(const glm::mat4& projView)
{
    const auto inv = glm::inverse(projView);

    std::vector<glm::vec3> frustumCorners;
    for (int x = 0; x < 2; ++x)
    {
        for (int y = 0; y < 2; ++y)
        {
            for (int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt = inv * glm::vec4(
                    2.0f * x - 1.0f,
                    2.0f * y - 1.0f,
                    2.0f * z - 1.0f,
                    1.0f
                );
                frustumCorners.emplace_back(pt / pt.w);
            }
        }
    }
    return frustumCorners;
}


CascadeView getLightView2(const glm::vec3& lightDir, int cascade) {
    float clipmapSize = ClipmapSizes[cascade];

    glm::vec3 clipmapWorldOrigin = glm::vec3(0);

    float texelSize = clipmapSize / static_cast<float>(ClipmapResolutions[cascade]);

    clipmapWorldOrigin.x = floor(clipmapWorldOrigin.x / texelSize) * texelSize;
    clipmapWorldOrigin.z = floor(clipmapWorldOrigin.z / texelSize) * texelSize;

    glm::vec3 up = abs(dot(lightDir, glm::vec3(0,1,0))) > 0.99f
        ? glm::vec3(0,0,1)
        : glm::vec3(0,1,0);

    float lightDistance = 1.0f;

    glm::mat4 lightView = glm::lookAt(
        clipmapWorldOrigin - lightDir * clipmapSize,
        clipmapWorldOrigin,
        up
    );

    glm::vec3 min = clipmapWorldOrigin - clipmapSize * 0.5f;
    glm::vec3 max = clipmapWorldOrigin + clipmapSize * 0.5f;

    auto corners = AABB::fromTo(min, max).corners();

    glm::vec3 lightMin(FLT_MAX);
    glm::vec3 lightMax(-FLT_MAX);
    for (int i = 0; i < 8; ++i) {
        glm::vec3 lightSpace = glm::vec3(lightView * glm::vec4(corners[i], 1.0f));
        lightMin = glm::min(lightMin, lightSpace);
        lightMax = glm::max(lightMax, lightSpace);
    }

    float zMargin = 0.f;

    float worldSizeX = lightMax.x - lightMin.x;
    float worldSizeY = lightMax.y - lightMin.y;
    float worldSize = std::max(worldSizeX, worldSizeY);

    float worldUnitsPerTexel = glm::max(
        (worldSizeX) / (float)ClipmapResolutions[cascade],
        (worldSizeY) / (float)ClipmapResolutions[cascade]
    );

    float expand = 0.f;// 8.f;

    glm::mat4 lightProj = glm::ortho(
        lightMin.x - expand, lightMax.x + expand,
        lightMin.y - expand, lightMax.y + expand,
        -lightMax.z - zMargin, -lightMin.z + zMargin
    );



    float shadowSoftnessWorld = 0.065f * ClipmapShadowSoftnessScale[cascade];

    float worldTexelSize = clipmapSize / static_cast<float>(ClipmapResolutions[cascade]);

    float filterRadiusUV = shadowSoftnessWorld / worldSize;

    GPUCamera lightCamera;
    lightCamera.projection = lightProj;
    lightCamera.view = lightView;
    lightCamera.nearClip = 0.f;
    lightCamera.farClip = clipmapSize;

    return {lightCamera, worldTexelSize, filterRadiusUV};
}

DirectionalLightCollection LightSystem::createDirectionalLightCollection(const glm::vec3 &direction,
    const glm::vec3 &color, float intensity, int resolution)
{
    TextureFactory tf(*renderer);

    std::array<TextureKey, 4> shadowMaps{};

    for (int i = 0; i < 4; ++i) {
        shadowMaps[i] =     tf.createTexture2D(TextureDescriptor2D(
            TexturePreset::SHADOW_2D_32F,
            ClipmapResolutions[i], ClipmapResolutions[i]
        ));
    }
    DirectionalLightCollection collection(direction, color, intensity, shadowMaps, 0);

    directionalLights.emplace_back();
    return collection;
}

BufferKey LightSystem::createDirectionalLightsBuffer(const GraphicsPassInvocationBase &invocation) const {
    auto dirLights = invocation.getVisiblePrimitives().getVisible<DirectionalLightCollection>();

    if (dirLights.empty()) return {};
    auto [dirLightBuffer, mappedDirLight] = invocation.getRenderer().getBufferStorage().createBufferWithData<GPUDirectionalLight>(dirLights.size(), BufferUsageHint::FRAME_SCRATCH_BUFFER);

    for (int i = 0; i < dirLights.size(); ++i) {
        auto& lightCollection = dirLights[i].getCollection();

        auto& dirLight = directionalLights[lightCollection->getIndex()];

        mappedDirLight[i] = dirLight;
        mappedDirLight[i].direction = glm::vec4(lightCollection->getDirection(), lightCollection->getIntensity());
        mappedDirLight[i].color = glm::vec4(lightCollection->getColor(), 1.0f);
    }
    return dirLightBuffer;
}

void DirectionalLightPass::onExecute(const Frame *frame, const PassInvocationID id) {
    LightSystem& lightResource = frame->getRenderer()->getSystem<LightSystem>();

    glEnable(GL_DEPTH_CLAMP);
    for (int cascade = 0; cascade < 1; ++cascade) {
        auto [lightView, worldTexelSize, worldFilterRadiusUV, constantBias, slopeBias] = getLightView2(light.getDirection(), cascade);

        auto& dirLight = lightResource.getDirectionalLight(light.getIndex());

        dirLight.worldOrigin = glm::vec4(0);
        dirLight.cascades[cascade].view = lightView.projection * lightView.view;
        dirLight.cascades[cascade].worldTexelSize = worldTexelSize;
        dirLight.cascades[cascade].worldFilterRadiusUV = worldFilterRadiusUV;
        dirLight.cascades[cascade].constantBias = constantBias;
        dirLight.cascades[cascade].slopeBias = slopeBias;
        dirLight.cascades[cascade].distance = ClipmapSizes[cascade];
        dirLight.cascades[cascade].texture = light.getShadowMaps()[cascade].id();
        dirLight.debugBits[0] = 0;

        RenderState state;
        state.cullMode = CullMode::Back;
        state.depthState.enabled = true;
        state.depthState.writeEnabled = true;
        state.depthState.func = DepthFunction::Less;
        state.blendState.enabled = false;

        GraphicsPassView view;
        view.view = lightView.view;
        view.projection = lightView.projection;

        auto depthPass = DepthPass(
            "DirLightShadowPass", pipeline, visible, RenderTexture::Depth(light.getShadowMaps()[cascade]), state, view
        );

        depthPass.onExecute(frame, id);
    }
}
