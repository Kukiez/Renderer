#pragma once
#include <unordered_map>
#include <Filesystem/Filepath.h>
#include <openGL/Shader/ShaderCache.h>
#include <openGL/Shader/ShaderClass.h>
#include <Renderer/Resource/Pass/RenderingPassKey.h>

class ShaderClass;

class MaterialDefaultParameters {
public:
    struct Parameter {
        char* data;
        size_t size;

        template <typename T>
        Parameter(const T& value) : data(new char[sizeof(T)]), size(sizeof(T)) {
            new (data) T(value);
        }
    };

    std::unordered_map<Filepath, Parameter, FilepathHash> parameters;
public:

    MaterialDefaultParameters(
        std::initializer_list<std::pair<std::string_view, Parameter>> init
    ) {
        for (const auto& [name, value] : init) {
            parameters.emplace(name, value);
        }
    }

    MaterialDefaultParameters(const MaterialDefaultParameters&) = delete;
    MaterialDefaultParameters& operator=(const MaterialDefaultParameters&) = delete;

    ~MaterialDefaultParameters() {
        for (auto& [name, data] : parameters) {
            delete[] data.data;
        }
    }

    auto& getParameters() const { return parameters; }
};

struct MaterialCreateInfo {
    Filepath name;
    Filepath vertex;
    Filepath material;
    MaterialDefaultParameters defaultParameters;
};

class VertexDefinition {
    Filepath name;
public:
    VertexDefinition(const Filepath &name) : name(name) {}

    const Filepath& getName() const { return name; }
};

class MaterialDefinition {
    const VertexDefinition* vertexDefinition{};
    const ShaderClass* materialClass{};
    char* defaultParametersBlock{};
public:
    MaterialDefinition(const ShaderClass& matClass, const VertexDefinition* vertexDef, char* defaultParametersBlock) : materialClass(&matClass), vertexDefinition(vertexDef), defaultParametersBlock(defaultParametersBlock) {}

    const ShaderClass* getMaterialClass() const { return materialClass; }

    std::string_view getName() const { return materialClass->name(); }

    const VertexDefinition* getVertexDefinition() const { return vertexDefinition; }

    char* getDefaultParameter(const ShaderClassMemberIndex index) {
        if ((size_t)index >= materialClass->size()) return nullptr;
        return defaultParametersBlock + (*materialClass)[index].offset;
    }

    char* getDefaultParametersBlock() const { return defaultParametersBlock; }
};

class MaterialCooking_t {
    ShaderCache shaderCache;
    std::unordered_map<Filepath, const MaterialDefinition*, FilepathHash> materials;
    std::unordered_map<Filepath, const VertexDefinition*, FilepathHash> vertices;
    size_t glslVersion = 460;
    std::vector<Filepath> enabledGLSLExtensions;
public:
    MaterialCooking_t() {
        enabledGLSLExtensions.emplace_back("GL_ARB_gpu_shader_int64");
    }

    LargeFilepath createFragmentShaderPermutation(const Filepath& vertex, const Filepath& material, const Filepath& pass);

    const MaterialDefinition* create(const MaterialCreateInfo& info);

    const MaterialDefinition* findMaterial(const Filepath& matName) const;

    static void importPass(RenderingPassType pass, const Filepath& path);

    static Filepath getPassPath(RenderingPassType pass);

    const Filepath& getBasePath();

    Filepath getFilepathForVertex(const Filepath& vertexName);

    void setGLSLVersion(size_t version) {
        glslVersion = version;
    }

    void enableGLSLExtension(const std::string_view ext) {
        enabledGLSLExtensions.emplace_back(ext);
    }
};

extern MaterialCooking_t MaterialCooking;