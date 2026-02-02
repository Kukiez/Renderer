#include <tbb/concurrent_vector.h>
#include "MaterialStorage.h"

#include <Renderer/Resource/Buffer/BufferComponentType.h>

#include "MaterialCooking.h"

void MaterialInstanceBuffer2::setParameter(ShaderClassMemberIndex paramIdx, const void *value) {
    auto& parameter = (*materialClass)[paramIdx];

    std::memcpy(memory + parameter.offset, value, parameter.size());
}

size_t MaterialBuffer::createInstance(const MaterialInstanceBuffer2 &matInst) {
    auto matClass = def->getMaterialClass();

    if (nextInstanceID >= buffer.sizeIn(matClass->size())) {
        buffer.resize(0, 0, buffer.size(), buffer.size() * 2);
    }
    auto mapped = buffer.mapRange<char>(nextInstanceID * matClass->size(), matClass->size());
    std::memcpy(mapped.data(), matInst.memory, matClass->size());
    return nextInstanceID++;
}

void MaterialKey2::compile(RenderingPassType pass) {
    for (auto& shader : buffer->compiledPasses) {
        if (shader.first == pass) return;
    }
    buffer->storage->compilePass(*this, pass);
}

ShaderKey MaterialKey2::getShaderPermutation(RenderingPassType pass) const {
    for (auto& [shaderType, shader] : buffer->compiledPasses) {
        if (shaderType == pass) return shader;
    }
    return {};
}

ShaderKey MaterialKey2::getOrCreateShaderPermutation(RenderingPassType pass) const {
    for (auto& [shaderType, shader] : buffer->compiledPasses) {
        if (shaderType == pass) return shader;
    }
    buffer->storage->compilePass(*this, pass);
    return getShaderPermutation(pass);
}

MaterialInstanceKey2 MaterialKey2::createInstance(const MaterialInstanceBuffer2 &matInstBuffer) {
    return MaterialInstanceKey2(*this, buffer->createInstance(matInstBuffer));
}

MaterialInstanceBuffer2 MaterialKey2::createInstanceBuffer() const {
    return buffer->storage->createInstanceBuffer(buffer->def->getMaterialClass(), buffer->def->getDefaultParametersBlock());
}

MaterialKey2 MaterialStorage::instantiate(std::string_view materialName, size_t capacity) {
    auto def = MaterialCooking.findMaterial(materialName);

    if (def) {
        auto clazz = def->getMaterialClass();

        MaterialBuffer buffer;
        buffer.buffer = bufferStorage->createBuffer(clazz->size() * capacity, BufferUsageHint::PERSISTENT_WRITE_ONLY);
        buffer.def = def;
        buffer.storage = this;
        buffer.nextInstanceID = 0;

        return MaterialKey2(new MaterialBuffer(buffer));
    }
    return {};
}

void MaterialStorage::compilePass(MaterialKey2 material, RenderingPassType pass) {
    if (!material) return;

    auto vertexDef = material.getDefinition()->getVertexDefinition();
    auto frag = MaterialCooking.createFragmentShaderPermutation(
        vertexDef->getName(),
        material.getDefinition()->getMaterialClass()->name().str(),
        MaterialCooking.getPassPath(pass)
    );

    auto basePath = MaterialCooking.getFilepathForVertex(vertexDef->getName());

    auto shader = ShaderFactory(shaderStorage).loadShader({
        .vertex = basePath,
        .fragment = frag + ".glsl"
    });
    material.buffer->compiledPasses.emplace_back(pass, shader);
}

void MaterialStorage::onFrameFinished(Renderer &renderer) {
    for (auto& tmp : tmpInstBuffers) {
        delete[] tmp;
    }
    tmpInstBuffers.clear();
}

MaterialInstanceBuffer2 MaterialStorage::createInstanceBuffer(const ShaderClass *matClass, const char *defaultParamBlock) {
    char* mem = new char[matClass->size()]; // TODO pool frame memory
    tmpInstBuffers.emplace_back(mem);
    std::memcpy(mem, defaultParamBlock, matClass->size());
    return MaterialInstanceBuffer2(matClass, mem);
}