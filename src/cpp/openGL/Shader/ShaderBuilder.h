#pragma once
#include "ShaderCache.h"
#include "ShaderDefinition.h"

class ShaderBuilder {
    ShaderCache& cache;
    mem::vector<ShaderBufferParameter> myBuffers{};
    mem::vector<ShaderUniformParameter> myParameters{};
    mem::vector<const ShaderClass*> myClasses{};
public:
    explicit ShaderBuilder(ShaderCache& cache) : cache(cache) {}

    void addPushConstant(std::string_view name, UniformParameterType type);

    void addBuffer(std::string_view name,
                   ShaderBuffer type = ShaderBuffer::Unknown,
                   BufferBindingIndex binding = BufferBindingIndex::INVALID,
                   ShaderBufferLayout layout = ShaderBufferLayout::STD430);

    void addClass(const ShaderClass& shaderClass);

    ShaderDefinition create() {
        return ShaderDefinition(std::move(myBuffers), std::move(myParameters), std::move(myClasses));
    }
};
