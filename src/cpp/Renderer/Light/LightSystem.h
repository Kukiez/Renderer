#pragma once
#include <Renderer/RenderingStages/LoadPass.h>
#include <Renderer/Renderer.h>
#include <Renderer/Pipeline/PassInvocation.h>
#include <Renderer/Scene/Primitives/PrimitivePipeline.h>
#include <Renderer/Scene/Primitives/VisiblePrimitive.h>

#include "LightCollection.h"
#include "PointLightPartition.h"

struct GPULightPrimitive {
    glm::vec4 position;
    glm::vec4 color;
    float radius;
    float intensity;
    int lightType;
    int shadowMap;
};

struct ShadowCascade {
    glm::mat4 view;
    float worldTexelSize;
    float worldFilterRadiusUV;
    float distance;
    glm::uint texture;
    float constantBias;
    float slopeBias;
    float pad2[2];
};

static constexpr auto MAX_CLIPMAP_LEVELS = 4;

struct GPUDirectionalLightRevamped {
    struct Clipmap {
        glm::vec4 origin{}; // .w = size
    };
    glm::vec4 direction;
    glm::vec4 color; //.w = intensity
    Clipmap clipmaps[8 * MAX_CLIPMAP_LEVELS];
};

struct GPUDirectionalLight {
    glm::vec4 direction;
    glm::vec4 color; //.w = intensity
    glm::vec4 worldOrigin;
    ShadowCascade cascades[4];
    unsigned debugBits[4]{};
};

struct GPULightClusterHeader {
    int totalLights;
    int totalClusters;
    int maxLightsPerCluster;
    int totalDirectionalLights;
    glm::ivec3 numClusters; // 16x9x24 - units
    int pad1;
    glm::ivec3 clusterSize; // in pixels
    int intersections;
};

struct GPULightCluster {
    int firstLight;
    int lightCount;
    int p[2];
};

struct GPULightIndices {
    int lightIndices[4];
};

struct GPUProbe {
    unsigned irradianceMap{};
    unsigned prefilterMap{};
    unsigned maxReflectionLOD{};
    unsigned pad;
};

struct GPUProbeHeader {
    unsigned probeCount;
    unsigned brdfLUT;
    unsigned pad[2];
};

class LightSystem {
    ShaderKey brdfLUTShader;

    TextureKey brdfLUT{};
    TextureKey shadowJitterTexture{};

    BufferKey iblProbeHeader;
    BufferKey iblProbes;

    TextureKey shadowContactShadows;

    std::vector<GPUDirectionalLight> directionalLights;

    Renderer* renderer;

    PointLightPartitionSystem lightClusterPartition = {};
public:
    RENDERERAPI void onLoad(RendererLoadView view);

    RENDERERAPI DirectionalLightCollection createDirectionalLightCollection(const glm::vec3& direction, const glm::vec3& color, float intensity, int resolution);

    GPUDirectionalLight& getDirectionalLight(unsigned index) { return directionalLights[index]; }

    RENDERERAPI BufferKey createDirectionalLightsBuffer(const GraphicsPassInvocationBase& invocation) const;

    PointLightPartitionSystem& getLightPartition() { return lightClusterPartition; }

    TextureKey getBRDFLUT() const { return brdfLUT; }
    TextureKey getShadowNoise() const { return shadowJitterTexture; }

    ShaderKey getBRDFLutShader() const { return brdfLUTShader; }
};

class DirectionalLightPass : public IPassInvocation<DirectionalLightPass> {
    DirectionalLightCollection light;
    const RenderingPipeline* pipeline;
    const VisiblePrimitiveList* visible;
public:
    RENDERERAPI void onExecute(const Frame* frame, PassInvocationID id);

    DirectionalLightPass(const DirectionalLightCollection& light, const RenderingPipeline* pipeline, const VisiblePrimitiveList* visible) : light(light), pipeline(pipeline), visible(visible) {}

};