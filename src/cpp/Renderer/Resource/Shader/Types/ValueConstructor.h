#pragma once
#include <openGL/Shader/ShaderReflection.h>

#include "ShaderStream.h"

struct ShaderValueConstructor {
    UniformParameterType type{};
    const void* value{};

    template <typename Stream>
    friend Stream& operator<<(Stream& s, const ShaderValueConstructor& v) {
        auto type = v.type;
        auto value = v.value;

        char buf[64]{};

        if (value == nullptr) {
            value = buf;
        }

        switch (type)
        {
            // ───────────────────────────────────────────────
            // Scalars
            // ───────────────────────────────────────────────
            case UniformParameterType::BOOL: {
                bool v = *static_cast<const bool*>(value);
                s << (v ? "true" : "false");
                break;
            }
            case UniformParameterType::INT: {
                int v = *static_cast<const int*>(value);
                s << "int(" << v << ")";
                break;
            }
            case UniformParameterType::UINT: {
                uint32_t v = *static_cast<const uint32_t*>(value);
                s << "uint(" << v << ")";
                break;
            }
            case UniformParameterType::FLOAT: {
                float v = *static_cast<const float*>(value);
                s << "float(" << v << ")";
                break;
            }

            // ───────────────────────────────────────────────
            // Float vectors
            // ───────────────────────────────────────────────
            case UniformParameterType::VEC2: {
                const auto f = static_cast<const float*>(value);
                s << "vec2(" << f[0] << ", " << f[1] << ")";
                break;
            }
            case UniformParameterType::VEC3: {
                const auto f = static_cast<const float*>(value);
                s << "vec3(" << f[0] << ", " << f[1] << ", " << f[2] << ")";
                break;
            }
            case UniformParameterType::VEC4: {
                const auto f = static_cast<const float*>(value);
                s << "vec4(" << f[0] << ", " << f[1] << ", " << f[2] << ", " << f[3] << ")";
                break;
            }

            // ───────────────────────────────────────────────
            // Integer vectors
            // ───────────────────────────────────────────────
            case UniformParameterType::IVEC2: {
                auto v = static_cast<const int*>(value);
                s << "ivec2(" << v[0] << ", " << v[1] << ")";
                break;
            }
            case UniformParameterType::IVEC3: {
                auto v = static_cast<const int*>(value);
                s << "ivec3(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
                break;
            }
            case UniformParameterType::IVEC4: {
                auto v = static_cast<const int*>(value);
                s << "ivec4(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
                break;
            }

            // ───────────────────────────────────────────────
            // Unsigned integer vectors
            // ───────────────────────────────────────────────
            case UniformParameterType::UVEC2: {
                auto v = static_cast<const uint32_t*>(value);
                s << "uvec2(" << v[0] << ", " << v[1] << ")";
                break;
            }
            case UniformParameterType::UVEC3: {
                auto v = static_cast<const uint32_t*>(value);
                s << "uvec3(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
                break;
            }
            case UniformParameterType::UVEC4: {
                auto v = static_cast<const uint32_t*>(value);
                s << "uvec4(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
                break;
            }

            // ───────────────────────────────────────────────
            // Matrices (row count × column count)
            // GLSL constructor order is column-major
            // ───────────────────────────────────────────────
            case UniformParameterType::MAT2: {
                auto f = static_cast<const float*>(value);
                s << "mat2("
                  << f[0] << ", " << f[1] << ", "
                  << f[2] << ", " << f[3] << ")";
                break;
            }
            case UniformParameterType::MAT3: {
                auto f = static_cast<const float*>(value);
                s << "mat3(";
                for (int i = 0; i < 9; i++) {
                    s << f[i];
                    if (i != 8) s << ", ";
                }
                s << ")";
                break;
            }
            case UniformParameterType::MAT4: {
                auto f = static_cast<const float*>(value);
                s << "mat4(";
                for (int i = 0; i < 16; i++) {
                    s << f[i];
                    if (i != 15) s << ", ";
                }
                s << ")";
                break;
            }

            case UniformParameterType::MAT2x3: { // 2 columns × 3 rows
                auto f = static_cast<const float*>(value);
                s << "mat2x3(";
                for (int i = 0; i < 6; i++) {
                    s << f[i];
                    if (i != 5) s << ", ";
                }
                s << ")";
                break;
            }
            case UniformParameterType::MAT2x4: {
                auto f = static_cast<const float*>(value);
                s << "mat2x4(";
                for (int i = 0; i < 8; i++) {
                    s << f[i];
                    if (i != 7) s << ", ";
                }
                s << ")";
                break;
            }
            case UniformParameterType::MAT3x2: {
                auto f = static_cast<const float*>(value);
                s << "mat3x2(";
                for (int i = 0; i < 6; i++) {
                    s << f[i];
                    if (i != 5) s << ", ";
                }
                s << ")";
                break;
            }
            case UniformParameterType::MAT3x4: {
                auto f = static_cast<const float*>(value);
                s << "mat3x4(";
                for (int i = 0; i < 12; i++) {
                    s << f[i];
                    if (i != 11) s << ", ";
                }
                s << ")";
                break;
            }
            case UniformParameterType::MAT4x2: {
                auto f = static_cast<const float*>(value);
                s << "mat4x2(";
                for (int i = 0; i < 8; i++) {
                    s << f[i];
                    if (i != 7) s << ", ";
                }
                s << ")";
                break;
            }
            case UniformParameterType::MAT4x3: {
                auto f = static_cast<const float*>(value);
                s << "mat4x3(";
                for (int i = 0; i < 12; i++) {
                    s << f[i];
                    if (i != 11) s << ", ";
                }
                s << ")";
                break;
            }

            // ───────────────────────────────────────────────
            // Samplers (cannot be constructed in GLSL)
            // ───────────────────────────────────────────────
            case UniformParameterType::SAMPLER_1D:
            case UniformParameterType::SAMPLER_2D:
            case UniformParameterType::SAMPLER_3D:
            case UniformParameterType::SAMPLER_CUBE:
            case UniformParameterType::SAMPLER_2D_ARRAY:
            case UniformParameterType::SAMPLER_CUBE_ARRAY:
            case UniformParameterType::SAMPLER_BUFFER:
            case UniformParameterType::SAMPLER_2D_SHADOW:
                s << "/* sampler cannot be constructed */";
                break;

            default:
                s << "/* unknown type */";
                break;
        }
        return s;
    }
};