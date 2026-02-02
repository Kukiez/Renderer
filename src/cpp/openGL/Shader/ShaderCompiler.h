#pragma once

#include <Filesystem/Filepath.h>

#include "Shader.h"
#include "ShaderDefinition.h"

class ShaderCache;

class ShaderCode {
    std::string data;
public:
    ShaderCode(std::string code) : data(std::move(code)) {}

    static ShaderCode open(std::string_view path);

    std::string_view code() const {
        return data;
    }

    const std::string& string() const {
        return data;
    }

    auto begin(this auto&& self) {
        return self.data.begin();
    }

    auto end(this auto&& self) {
        return self.data.end();
    }

    bool empty() const {
        return data.empty();
    }
};

struct FileIncludeResolver {
    static std::string open(const Filepath& file);
    static std::string openAndGuardIncludes(const Filepath& file);
};

class ShaderProgram {
    ShaderDefinition myDefinition{};
    unsigned myID = 0;
    ShaderPipelineDescriptor shaders{};
public:
    ShaderProgram() = default;

    ShaderProgram(const unsigned id, ShaderDefinition&& def, const ShaderPipelineDescriptor &desc) : myDefinition(std::move(def)), myID(id), shaders(desc) {}

    auto& definition() const {
        return myDefinition;
    }

    auto& definition() {
        return myDefinition;
    }

    unsigned id() const {
        return myID;
    }

    glm::vec3 threads() const {
        if (shaders.compute) {
            return shaders.compute->getThreads();
        }
        return glm::vec3();
    }

    UniformParameterIndex getUniformParameter(size_t hash) const;
    UniformParameterIndex getUniformParameter(const ShaderString& str) const;

    const ShaderUniformParameter* getUniform(UniformParameterIndex index)const;
};

static inline auto INVALID_SHADER_PROGRAM = ShaderProgram();

class ShaderCompiler {
protected:
    ShaderCache& cache;
public:
    virtual ~ShaderCompiler() = default;

    explicit ShaderCompiler(ShaderCache& cache) : cache(cache) {}

    virtual ShaderCode parseShaderFile(std::string_view filePath);

    virtual ShaderDefinition compileShaderFile(const ShaderCode& code) = 0;

    virtual unsigned createShader(const ShaderCode& code, ShaderProgramType type) = 0;

    virtual ShaderProgram createShaderProgramDefinition(const ShaderPipelineDescriptor &shaders) = 0;

    virtual ShaderProgram& initializeShaderProgram(ShaderProgram& program, const ShaderPipelineDescriptor& shaders) = 0;
};

class ShaderPipelineBuilder {
    ShaderCache& cache;
    ShaderCompiler& compiler;
    ShaderPipelineDescriptor shaders{};
public:
    ShaderPipelineBuilder(ShaderCache& cache, ShaderCompiler& compiler) : cache(cache), compiler(compiler) {}

    const Shader* getOrCompileShader(std::string_view path, ShaderProgramType type) const;

    const ComputeShader* getOrCompileShader(std::string_view path, glm::vec3 threads) const;

    ShaderPipelineBuilder& withVertex(const std::string_view vertex) {
        shaders.vertex = getOrCompileShader(vertex, ShaderProgramType::VERTEX);
        return *this;
    }

    ShaderPipelineBuilder& withFragment(const std::string_view fragment) {
        shaders.fragment = getOrCompileShader(fragment, ShaderProgramType::FRAGMENT);
        return *this;
    }

    ShaderPipelineBuilder& withGeometry(const std::string_view geometry) {
        shaders.geometry = getOrCompileShader(geometry, ShaderProgramType::GEOMETRY);
        return *this;
    }

    ShaderPipelineBuilder& withCompute(const std::string_view compute, const glm::vec3& threads) {
        shaders.compute = getOrCompileShader(compute, threads);
        return *this;
    }

    ShaderPipelineBuilder& withTessellation(const std::string_view tesControl, const std::string_view tesEvaluation) {
        shaders.tessControl = getOrCompileShader(tesControl, ShaderProgramType::TESS_CONTROL);
        shaders.tessEval = getOrCompileShader(tesEvaluation, ShaderProgramType::TESS_EVALUATION);
        return *this;
    }
    auto build() const {
        return shaders;
    }
};