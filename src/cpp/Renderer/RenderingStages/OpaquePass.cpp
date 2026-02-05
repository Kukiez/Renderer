#include "OpaquePass.h"

#include <Renderer/Graphics/GraphicsContext.h>
#include <Renderer/Graphics/PassBindings.h>
#include <Renderer/Light/AtmosphereSystem.h>
#include <Renderer/Light/LightSystem.h>
#include <Renderer/Pipeline/PassInvocation.h>
#include <Renderer/Resource/Texture/TextureQuery.h>
#include <Renderer/Skybox/SkyboxPass.h>

static constexpr int LIGHTS_INDEX = 50;
static constexpr int LIGHT_INDICES_INDEX = 51;
static constexpr int LIGHT_CLUSTERS_INDEX = 52;
static constexpr int LIGHT_HEADER_INDEX = 53;
static constexpr int IBL_PROBE_HEADER_INDEX = 54;
static constexpr int IBL_PROBES_INDEX = 55;
static constexpr int SHADOW_MAPS_INDEX = 56;
static constexpr int DIRECTIONAL_LIGHTS_INDEX = 57;
static constexpr int CAMERA_INDEX = 58;
static constexpr int MATERIALS_INDEX = 59;
static constexpr int ATMOSPHERE_INDEX = 60;

struct LightPass {
    int numPointLights;
    unsigned brdfLUT;
    unsigned shadowNoise;
};

void OpaquePass::onPassBegin(const GraphicsPassInvocationBase& invocation) {
    auto& renderer = invocation.getRenderer();

    auto& lightSystem = renderer.getSystem<LightSystem>();
    auto& lightPartition = lightSystem.getLightPartition();

    BufferKey pointLightsBuffer = lightPartition.createPointLightsBuffer(invocation);
    lightPartition.renderLightList(invocation, pointLightsBuffer);

    auto [lightPassBuffer, lightPassData] = invocation.getRenderer().getBufferStorage().createBufferWithData<LightPass>(1, BufferUsageHint::FRAME_SCRATCH_BUFFER);

    lightPassData[0].numPointLights = pointLightsBuffer.sizeIn<GPUPointLight>();
    lightPassData[0].brdfLUT = lightSystem.getBRDFLUT().id();
    lightPassData[0].shadowNoise = lightSystem.getShadowNoise().id();

    auto dirLights = lightSystem.createDirectionalLightsBuffer(invocation);
    static GPUBiomeAtmosphere gpuAtmosphere{};
    gpuAtmosphere.skybox.envCubemap = skybox.envCubemap.id();
    gpuAtmosphere.skybox.irradianceMap = skybox.irradianceMap.id();
    gpuAtmosphere.skybox.prefilterMap = skybox.prefilterMap.id();
    gpuAtmosphere.skybox.prefilterLOD = skybox.prefilterLOD;

    GraphicsContext ctx(&renderer);

    TextureQuery tq(renderer);

    BufferBindingsSet set(renderer.getRenderAllocator(), 10);

    auto [atmosphere, buf] = invocation.getRenderer().getBufferStorage().createBufferWithData<GPUBiomeAtmosphere>(1, BufferUsageHint::FRAME_SCRATCH_BUFFER);
    buf[0] = gpuAtmosphere;

    set.add({
        .buffer = lightPassBuffer,
        .index = 49
    });

    if (pointLightsBuffer.size() != 0) set.add({
        .buffer = pointLightsBuffer,
        .index = LIGHTS_INDEX
    });

    set.add({
        .buffer = lightPartition.lightIndices,
        .index = LIGHT_INDICES_INDEX
    });

    set.add({
        .buffer = lightPartition.clusterHeader,
        .index = LIGHT_HEADER_INDEX
    });

    if (dirLights != NULL_BUFFER_KEY) set.add({
        .buffer = dirLights,
        .index = DIRECTIONAL_LIGHTS_INDEX
    });

    set.add({
        .buffer = invocation.getViewBuffer(),
        .index = CAMERA_INDEX
    });

    set.add({
        .buffer = tq.getMaterial2DBuffer(),
        .index = MATERIALS_INDEX
    });

    set.add({
        .buffer = atmosphere,
        .index = ATMOSPHERE_INDEX
    });

    set.bind(ctx);
}

void OpaquePass::onPassEnd(const GraphicsPassInvocationBase &invocation) {
}

void TransparentRenderingPass::onPassBegin(const GraphicsPassInvocationBase &invocation) {
}

void TransparentRenderingPass::onPassEnd(const GraphicsPassInvocationBase &invocation) {
}
