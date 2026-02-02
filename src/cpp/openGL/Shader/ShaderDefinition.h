#pragma once
#include "ShaderClass.h"
#include "ShaderReflection.h"

class ShaderDefinition {
    mem::vector<ShaderBufferParameter> myBuffers{};
    mem::vector<const ShaderClass*> myClasses{};
    ShaderUniformLayout uniformLayout{};
public:
    ShaderDefinition() = default;

    ShaderDefinition(mem::vector<ShaderBufferParameter>&& buffers,
        mem::vector<ShaderUniformParameter>&& params, mem::vector<const ShaderClass*>&& classes) : myBuffers(std::move(buffers)), uniformLayout(std::move(params)), myClasses(std::move(classes)) {}

    auto& buffers() const {
        return myBuffers;
    }

    auto& parameters() {
        return uniformLayout;
    }

    auto& parameters() const {
        return uniformLayout;
    }

    std::pair<BufferBindingIndex, int> getBufferBinding(std::string_view buffer) const;

    friend std::ostream& operator<<(std::ostream& os, const ShaderDefinition& def);

    ShaderDefinition& operator+=(const ShaderDefinition& other);
};