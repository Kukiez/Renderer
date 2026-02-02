#pragma once

#include "ShaderCompiler.h"
#include "ShaderDefinition.h"

class OpenGLShaderCompiler final : public ShaderCompiler {
public:
    using ShaderCompiler::ShaderCompiler;

    ShaderDefinition compileShaderFile(const ShaderCode& code) override;

    unsigned createShader(const ShaderCode& code, ShaderProgramType type) override;

    ShaderProgram createShaderProgramDefinition(const ShaderPipelineDescriptor &shaders) override;

    ShaderProgram& initializeShaderProgram(ShaderProgram& program, const ShaderPipelineDescriptor& shaders) override;
};
