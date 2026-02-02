#pragma once
#include <Renderer/Graphics/DrawContainers/ComputeCmdContainer.h>
#include <Renderer/Resource/Buffer/BufferComponentType.h>
#include <Renderer/Resource/Shader/ShaderComponentType.h>

struct LightClusterHeader {
    glm::ivec3 clusterCount;
};

struct GPUPointLight {
    glm::vec4 position;      // .xyz, .range
    glm::vec4 color;         // .rgb, .intensity

    int shadowMapIndex; // -1 if none
    int shadowSoftness;
    float shadowBias;

    int _;
};


struct PointLightPartitionSystem {
    static constexpr auto MAX_LIGHTS_PER_TILE = 4;

    BufferKey lightIndices{};
    BufferKey clusterHeader{};

    ShaderKey clusterBuildShader{};

    LightClusterHeader header;

    void initialize(Renderer& renderer) {
        auto& bufferStorage = renderer.getBufferStorage();

        static constexpr auto ClusterCount = glm::ivec3(12, 9, 24);

        header.clusterCount = ClusterCount;

        int totalClusters = ClusterCount.x * ClusterCount.y * ClusterCount.z;

        lightIndices = bufferStorage.createBuffer<int[MAX_LIGHTS_PER_TILE]>(totalClusters, BufferUsageHint::IMMUTABLE);
        auto [clusterBuf, clusterData] = bufferStorage.createBufferWithData<LightClusterHeader>(totalClusters, BufferUsageHint::IMMUTABLE);

        clusterData[0] = header;
        clusterHeader = clusterBuf;

        clusterBuildShader = ShaderFactory(renderer).loadComputeShader({
            .compute = "shaders/Lighting/point_light_clustering.glsl",
            .threads = glm::vec3(1, 1, 1)
        });
    }

    BufferKey createPointLightsBuffer(const GraphicsPassInvocationBase& invocation) {
        auto visiblePointLights = invocation.getVisiblePrimitives().getVisible<PointLightCollection>();

        auto [pointLightsBuffer, pointLightsData] = invocation.getRenderer().getBufferStorage().createBufferWithData<GPUPointLight>(visiblePointLights.size(), BufferUsageHint::FRAME_SCRATCH_BUFFER);

        size_t i = 0;
        for (auto lightPrim : visiblePointLights) {
            auto& light = lightPrim.getCollection();
            auto& gpuLight = pointLightsData[i];

            gpuLight.color = glm::vec4(light->getColor(), light->getIntensity());
            gpuLight.position = glm::vec4(light.getWorldTransform().position, light->getRadius());
            gpuLight.shadowMapIndex = 0;
        }
        return pointLightsBuffer;
    }

    void renderLightList(const GraphicsPassInvocationBase& invocation, BufferKey pointLightsBuffer) {
        auto& renderer = invocation.getRenderer();

        if (pointLightsBuffer.size() == 0) return;

        RenderPass renderPass(&renderer);

        auto& pass = renderPass.createGraphicsPass("ClusterBuildPass", clusterBuildShader);

        pass.bind(lightIndices, "OutPointLightIndices");
        pass.bind(clusterHeader, "ClusterHeader");
        pass.bind(pointLightsBuffer, "PointLightsBuffer");

        const auto camera = invocation.getViewBuffer();
        pass.bind(camera, "CameraBuffer");
        pass.push("pointLightCount", static_cast<int>(pointLightsBuffer.sizeIn<GPUPointLight>()));

        auto& container = pass.usingDrawContainer<ComputeCmdContainer>();

        container.dispatch(header.clusterCount.x, header.clusterCount.y, header.clusterCount.z);

        renderer.render(renderPass);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
};