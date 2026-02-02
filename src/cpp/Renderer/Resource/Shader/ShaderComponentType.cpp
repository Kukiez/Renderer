#include "ShaderComponentType.h"
#include <openGL/Shader/OpenGLShaderCompiler.h>
#include <Renderer/Renderer.h>

#include "ShaderFactory.h"
#include "ShaderQuery.h"

ShaderComponentType::ShaderComponentType() {
    shaders.emplace_back();
}

void ShaderComponentType::synchronize() {
    OpenGLShaderCompiler compiler(cache);

    for (auto& [key, pipeline] : toInitializeShaders) {
        compiler.initializeShaderProgram(shaders[key.id()], pipeline);
    }
    toInitializeShaders = {};
}

ShaderKey ShaderComponentType::createShader(const ShaderDescriptor &descriptor, bool immediate = false) {
    OpenGLShaderCompiler compiler(cache);

    ShaderPipelineBuilder builder(cache, compiler);

    builder.withFragment(descriptor.fragment)
           .withVertex(descriptor.vertex)
           .withGeometry(descriptor.geometry)
           .withTessellation(descriptor.tessControl, descriptor.tessEvaluation);

    const ShaderPipelineDescriptor desc = builder.build();

    shaders.emplace_back(compiler.createShaderProgramDefinition(desc));

    ShaderKey key{shaders.size() - 1};

    if (immediate) {
        compiler.initializeShaderProgram(shaders.back(), desc);
    } else {
        toInitializeShaders.emplace_back(key, desc); // TODO strings are passed by string_view, lifetime
    }
    return key;
}

ShaderKey ShaderComponentType::createComputeShader(const ComputeShaderDescriptor &descriptor, bool immediate = false) {
    OpenGLShaderCompiler compiler(cache);

    ShaderPipelineBuilder builder(cache, compiler);

    builder.withCompute(descriptor.compute, descriptor.threads);

    const ShaderPipelineDescriptor desc = builder.build();

    shaders.emplace_back(compiler.createShaderProgramDefinition(desc));

    auto key = ShaderKey{shaders.size() - 1};

    if (immediate) {
        compiler.initializeShaderProgram(shaders.back(), desc);
    } else {
        toInitializeShaders.emplace_back(key, desc); // TODO strings are passed by string_view, lifetime
    }

    return key;
}

/*
 *
 *
 */

void ShaderFactory::setNamedShader(ShaderComponentType *shaders, ShaderKey key, ComponentIndex name,
    const mem::type_info *type)
{
    ComponentField<NamedShaderField> field;
    field.type = type;
    field.shader = key;
}

ShaderFactory::ShaderFactory(Renderer &renderer) : type(&renderer.getShaderStorage()) {}

ShaderKey ShaderFactory::loadShader(const ShaderDescriptor &descriptor) const {
    return type->createShader(descriptor, false);
}

ShaderKey ShaderFactory::loadComputeShader(const ComputeShaderDescriptor &descriptor) const {
    return type->createComputeShader(descriptor, false);
}

ShaderQuery::ShaderQuery(Renderer &renderer) : type(&renderer.getShaderStorage()) {}

const ShaderProgram & ShaderQuery::getShaderProgram(const ShaderKey key) const {
    return type->getShader(key);
}

const ShaderClass & ShaderQuery::findShaderClass(std::string_view name) const {
    return type->cache.findShaderClass(name);
}
