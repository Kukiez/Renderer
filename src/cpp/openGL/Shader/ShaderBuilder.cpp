#include "ShaderBuilder.h"

void ShaderBuilder::addPushConstant(const std::string_view name, UniformParameterType type) {
    ShaderUniformParameter param{};
    param.name = cache.internString(name);
    param.type = type;

    for (int i = 0; i < myParameters.size(); i++) {
        if (myParameters[i].name == name) {
            return;
        }
    }
    myParameters.emplace_back(param);
}

void ShaderBuilder::addBuffer(const std::string_view name, ShaderBuffer type, BufferBindingIndex binding,
    ShaderBufferLayout layout) {
    auto interned = cache.internString(name);

    for (int i = 0; i < myBuffers.size(); ++i) {
        if (myBuffers[i].name == interned) {
            return;
        }
    }
    myBuffers.emplace_back(interned, type, binding, layout);
}

void ShaderBuilder::addClass(const ShaderClass &shaderClass) {
    myClasses.emplace_back(&shaderClass);
}
