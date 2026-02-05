#include "PushConstant.h"

#include <openGL/Shader/ShaderCompiler.h>
#include <openGL/Shader/ShaderReflection.h>
#include <Renderer/Resource/Texture/TextureKey.h>
#include <Renderer/Resource/Texture/TextureQuery.h>

#include "GraphicsContext.h"
#include <Renderer/Renderer.h>

void PushConstantSet::push(const UniformParameterIndex &index, const void *push, size_t pushBytes) {
    auto& uniformLayout = shader->definition().parameters();

    const ShaderUniformParameter& param = uniformLayout[index];

    const size_t offset = param.offset;
    size_t align = 0;
    size_t bytes = getUniformSizeAndAlign(param.type, align);

    const size_t minBytes = std::min(pushBytes, bytes);
    std::memcpy(block + offset, push, minBytes);

    usedSlots.set(static_cast<size_t>(index));
}

UniformParameterType PushConstantSet::push(const ShaderString &str, const void *pushData, size_t pushBytes) {
    auto& uniformLayout = shader->definition().parameters();

    const auto index = uniformLayout.getUniformParameter(str.hash);
    if (index != UniformParameterIndex::INVALID) {
        push(index, pushData, pushBytes);
    }
    return {};
}

PushConstantSet PushConstantSet::allocate(GraphicsAllocator *allocator, const ShaderProgram *shader) {
    struct alignas(16) char_align_16_t {};

    size_t bytesReq = shader->definition().parameters().totalBytesUsed();
    return {
        shader,
        (char*)allocator->allocate<char_align_16_t>(
            bytesReq / 16 + (bytesReq % 16 != 0)
        )
    };
}

bool PushConstantSet::bind(const GraphicsContext &graphics, const ShaderProgram *shader, const UniformParameterIndex &uniform,
    const void *data)
{
    if (uniform == UniformParameterIndex::INVALID) return false;

    auto& uniformLayout = shader->definition().parameters();
    return bind(graphics, shader, uniformLayout[uniform], data);
}

bool PushConstantSet::bind(const GraphicsContext &graphics, const ShaderProgram *shader, const std::string_view uniform,
    const void *data) {
    const ShaderString str = ShaderString::fromString(uniform);
    return bind(graphics, shader, str, data);
}

bool PushConstantSet::bind(const GraphicsContext &graphics, const ShaderProgram *shader, const ShaderString &uniform,
    const void *data) {
    auto param = shader->definition().parameters().getUniformParameter(uniform.hash);
    return bind(graphics, shader, param, data);
}

bool PushConstantSet::bind(const GraphicsContext &graphics, const ShaderProgram *shader,
    const ShaderUniformParameter &uniform, const void *data)
{
    auto& [name, type, location, offset] = uniform;

    const auto* fdata = static_cast<const float *>(data);
    const auto* sampler = static_cast<const SamplerKey*>(data);

    switch (type) {
        case UniformParameterType::INT:
        case UniformParameterType::BOOL:
            glUniform1iv(location, 1, static_cast<const int*>(data));
            break;
        case UniformParameterType::UINT:
            glUniform1uiv(location, 1, static_cast<const unsigned*>(data));
            break;

        case UniformParameterType::IVEC2:
            glUniform2iv(location, 1, static_cast<const int*>(data));
            break;
        case UniformParameterType::IVEC3:
            glUniform3iv(location, 1, static_cast<const int*>(data));
            break;
        case UniformParameterType::IVEC4:
            glUniform4iv(location, 1, static_cast<const int*>(data));
            break;

        case UniformParameterType::UVEC2:
            glUniform2uiv(location, 1, static_cast<const unsigned int*>(data));
            break;
        case UniformParameterType::UVEC3:
            glUniform3uiv(location, 1, static_cast<const unsigned int*>(data));
            break;
        case UniformParameterType::UVEC4:
            glUniform4uiv(location, 1, static_cast<const unsigned int*>(data));
            break;

        case UniformParameterType::FLOAT:
            glUniform1fv(location, 1, fdata);
            break;
        case UniformParameterType::VEC2:
            glUniform2fv(location, 1, fdata);
            break;
        case UniformParameterType::VEC3:
            glUniform3fv(location, 1, fdata);
            break;
        case UniformParameterType::VEC4:
            glUniform4fv(location, 1, fdata);
            break;

        case UniformParameterType::MAT2:
            glUniformMatrix2fv(location, 1, GL_FALSE, fdata);
            break;
        case UniformParameterType::MAT3:
            glUniformMatrix3fv(location, 1, GL_FALSE, fdata);
            break;
        case UniformParameterType::MAT4:
            glUniformMatrix4fv(location, 1, GL_FALSE, fdata);
            break;

        case UniformParameterType::SAMPLER_2D:
        case UniformParameterType::SAMPLER_CUBE:
        case UniformParameterType::SAMPLER_2D_ARRAY:
        case UniformParameterType::SAMPLER_3D:
        case UniformParameterType::SAMPLER_2D_SHADOW:
        {
            TextureQuery texQ(*graphics.getRenderer());

            if (!texQ.isValid(*sampler)) {
                std::cout << "INVALID TEXTURE FOR SAMPLER UNIFORM IN UNIFORM: " << uniform.name << std::endl;
                return false;
            }
            glUniformHandleui64ARB(location, texQ.getTextureGPUHandle(*sampler));
            break;
        }
        default: assert(false);
    }
    return true;
}

bool PushConstantSet::bind(const GraphicsContext &graphics) const {
    auto& uniformLayout = shader->definition().parameters();

    size_t i = 0;
    for (auto& [name, type, location, offset] : uniformLayout) {
        if (usedSlots.test(i)) {
            if (!bind(graphics, shader, static_cast<UniformParameterIndex>(i), block + offset)) {
                return false;
            }
        }
        ++i;
    }

    return true;
}

template <typename T>
void PushConstantSet::push(std::string_view name, const T& value) {
    using UType = std::decay_t<T>;
    ShaderString str = ShaderString::fromString(name);

    if constexpr (std::is_same_v<UType, TextureKey>) {
        auto& uniformLayout = shader->definition().parameters();
        auto id = value.id();
        SamplerKey sampler = value.sampler();
        auto v = push(str, &sampler, sizeof(SamplerKey));
    } else {
        auto v = push(str, &value, sizeof(UType));
    }
}

#define PUSH_CONSTANT(T) template void PushConstantSet::push<T>(std::string_view, const T&);

PUSH_CONSTANT(glm::mat4)
PUSH_CONSTANT(glm::vec4)
PUSH_CONSTANT(glm::vec3)
PUSH_CONSTANT(glm::vec2)
PUSH_CONSTANT(float)
PUSH_CONSTANT(int)
PUSH_CONSTANT(bool)
PUSH_CONSTANT(unsigned int)
PUSH_CONSTANT(unsigned short)
PUSH_CONSTANT(glm::ivec4)
PUSH_CONSTANT(glm::ivec3)
PUSH_CONSTANT(glm::ivec2)
PUSH_CONSTANT(glm::uvec4)
PUSH_CONSTANT(glm::uvec3)
PUSH_CONSTANT(glm::uvec2)
PUSH_CONSTANT(TextureKey)
PUSH_CONSTANT(SamplerKey)
PUSH_CONSTANT(unsigned long long)