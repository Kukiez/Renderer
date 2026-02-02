#pragma once
#include <ECS/Level/LevelContext.h>
#include <openGL/Shader/ShaderCache.h>
#include <openGL/Shader/ShaderCompiler.h>
#include <openGL/Shader/Shader.h>
#include "ShaderKey.h"
#include "ShaderFactory.h"

struct ShaderDescriptor;

struct NamedShaderField {
    ShaderKey shader{};

    template <typename T>
    static NamedShaderField of(const ShaderKey shader) {
        return NamedShaderField{shader};
    }
};

class ShaderComponentType {
    friend class ShaderFactory;
    friend class ShaderQuery;

    ShaderCache cache{};
    tbb::concurrent_vector<ShaderProgram> shaders{};
    tbb::concurrent_vector<std::pair<ShaderKey, ShaderPipelineDescriptor>> toInitializeShaders{};

    std::atomic<unsigned> nextKey = 1;
public:
    ShaderComponentType();

    ShaderComponentType(const ShaderComponentType&) = delete;
    ShaderComponentType(ShaderComponentType&&) = delete;

    void synchronize();

    ShaderProgram& getShader(const ShaderKey key) {
        return shaders[key.id()];
    }

    ShaderKey createShader(const ShaderDescriptor &descriptor, bool immediate);
    ShaderKey createComputeShader(const ComputeShaderDescriptor &descriptor, bool immediate);

    ShaderCache& getShaderCache() {
        return cache;
    }

    const ShaderCache& getShaderCache() const {
        return cache;
    }
};