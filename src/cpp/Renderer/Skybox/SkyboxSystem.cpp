#include "SkyboxSystem.h"
#include <Renderer/RenderingStages/LoadPass.h>
#include <Renderer/Graphics/RenderPass.h>
#include <Renderer/Resource/Geometry/GeometryBuilder.h>
#include <Renderer/Resource/Shader/ShaderFactory.h>

void SkyboxRenderer::onLoad(RendererLoadView view) {
    ShaderFactory shaderFactory(view.getRenderer());

    skyboxShader = shaderFactory.loadShader({
        .vertex = "shaders/Skybox/skybox_vertex.glsl",
        .fragment = "shaders/Skybox/skybox_frag.glsl"
    });

    hdriLoadShader = shaderFactory.loadShader({
        .vertex = "shaders/Lighting/hdri_vertex_load.glsl",
        .fragment = "shaders/Lighting/hdri_frag_load.glsl"
    });
    irradianceMapShader = shaderFactory.loadShader({
        .vertex = "shaders/Lighting/hdri_vertex_load.glsl",
        .fragment = "shaders/Lighting/hdri_irradiance_frag.glsl"
    });
    prefilterMapShader = shaderFactory.loadShader({
        .vertex = "shaders/Lighting/hdri_vertex_load.glsl",
        .fragment = "shaders/Lighting/ibl_prefilter_frag.glsl"
    });

    GeometryFactory geoFactory(view.getRenderer());

    const GeometryDescriptor cubemapGeometryDesc(
        Skybox::CubemapVertices,
        VertexLayoutOf(VertexAttrib::Position3)
    );

    cubemapGeometryKey = geoFactory.loadGeometry(cubemapGeometryDesc);
}
