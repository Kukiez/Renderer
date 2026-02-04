#pragma once
#include <glm/ext/matrix_transform.hpp>
#include <Renderer/RenderingStages/LoadPass.h>
#include <Renderer/Resource/Geometry/GeometryKey.h>
#include <Renderer/Resource/Shader/ShaderKey.h>
#include <Renderer/Resource/Texture/TextureKey.h>
#include "RendererAPI.h"

struct Skybox {
    struct CubemapVertex {
        float x, y, z;
    };

    static constexpr CubemapVertex CubemapVertices[] = {
        { -1.0f,  1.0f, -1.0f }, { -1.0f, -1.0f, -1.0f }, {  1.0f, -1.0f, -1.0f },
        {  1.0f, -1.0f, -1.0f }, {  1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f },

        { -1.0f, -1.0f,  1.0f }, { -1.0f, -1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f },
        { -1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f,  1.0f }, { -1.0f, -1.0f,  1.0f },

        {  1.0f, -1.0f, -1.0f }, {  1.0f, -1.0f,  1.0f }, {  1.0f,  1.0f,  1.0f },
        {  1.0f,  1.0f,  1.0f }, {  1.0f,  1.0f, -1.0f }, {  1.0f, -1.0f, -1.0f },

        { -1.0f, -1.0f,  1.0f }, { -1.0f,  1.0f,  1.0f }, {  1.0f,  1.0f,  1.0f },
        {  1.0f,  1.0f,  1.0f }, {  1.0f, -1.0f,  1.0f }, { -1.0f, -1.0f,  1.0f },

        { -1.0f,  1.0f, -1.0f }, {  1.0f,  1.0f, -1.0f }, {  1.0f,  1.0f,  1.0f },
        {  1.0f,  1.0f,  1.0f }, { -1.0f,  1.0f,  1.0f }, { -1.0f,  1.0f, -1.0f },

        { -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f,  1.0f }, {  1.0f, -1.0f, -1.0f },
        {  1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f,  1.0f }, {  1.0f, -1.0f,  1.0f }
    };

    static inline std::array HDRIViews = std::array{
        glm::lookAt(glm::vec3(0), glm::vec3(0) + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0) + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0) + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0) + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0) + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0) + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0))
    };

    TextureKey envCubemap{};
    TextureKey irradianceMap{};
    TextureKey prefilterMap{};
    int prefilterLOD = 0;

    Skybox() = default;
};

struct GPUSkybox {
    unsigned envCubemap;
    unsigned irradianceMap;
    unsigned prefilterMap;
    int prefilterLOD;
};

struct RENDERERAPI SkyboxRenderer {
    GeometryKey cubemapGeometryKey;
    ShaderKey skyboxShader;

    ShaderKey brdfLutShader;
    ShaderKey hdriLoadShader;
    ShaderKey irradianceMapShader;
    ShaderKey prefilterMapShader;

    TextureKey hdriTexture{};

    void onLoad(RendererLoadView view);
};
