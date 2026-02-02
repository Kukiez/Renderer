#pragma once
#include <glm/vec3.hpp>

struct ShaderDescriptor {
    std::string_view vertex{};
    std::string_view fragment{};
    std::string_view geometry{};
    std::string_view tessControl{};
    std::string_view tessEvaluation{};
};

struct ComputeShaderDescriptor {
    std::string_view compute{};
    glm::vec3 threads{};
};

struct ShaderGraphDescriptor {
    std::string_view vertex{};
    std::string_view fragment{};
    std::string_view geometry{};
    std::string_view tessControl{};
    std::string_view tessEvaluation{};
};

struct ShaderMaterialDescriptor {
    std::string_view material{};            /* path to material.glsl */
    std::string_view instanceClass{};       /* struct name for the material */
    std::string_view instanceFunction{};    /* function name for applying the material | void(inout Surface surface, inout Material material) */
};

struct ShaderGeometryDescriptor {
    std::string_view geometry{};            /* path to geometry.glsl */
    std::string_view geometryClass{};       /* geometry instance struct */
};