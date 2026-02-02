#pragma once
#include <Renderer/RenderingStages/LoadPass.h>
#include <Renderer/Resource/Shader/ShaderFactory.h>


struct DisplayResources {
    ShaderKey defaultDisplayShader;
    ShaderKey depthDisplayShader;

    void onLoad(RendererLoadView renderer) {
        ShaderFactory sf(renderer.getRenderer());

        defaultDisplayShader = sf.loadShader({
            .vertex = "shaders/screen/screen_vertex.glsl",
            .fragment = "shaders/screen/display_frag.glsl"
        });

        depthDisplayShader = sf.loadShader({
            .vertex = "shaders/screen/screen_vertex.glsl",
            .fragment = "shaders/screen/debug_display_depth.glsl"
        });
    }
};
