#pragma once
#include <Filesystem/Filepath.h>
#include <memory/atomic_byte_arena.h>
#include <Renderer/Resource/Buffer/BufferKey.h>
#include <Renderer/Resource/Pass/RenderingPassKey.h>
#include <Renderer/Resource/Shader/ShaderComponentType.h>
#include "MaterialKey.h"

class MaterialStorage;
class MaterialDefinition;

class MaterialCache {
    std::unordered_map<Filepath, const ShaderClass*, FilepathHash> materialToClass;

public:
    const ShaderClass* findMaterialClass(const Filepath& material) const {
        const auto it = materialToClass.find(material);
        if (it == materialToClass.end()) {
            return nullptr;
        }
        return it->second;
    }
};

class MaterialStorage {
    MaterialCache cache;
    std::unordered_map<Filepath, MaterialKey2, FilepathHash> nameToMaterial;
    BufferResourceStorage* bufferStorage;
    ShaderComponentType* shaderStorage;

    tbb::concurrent_vector<char*> tmpInstBuffers;
public:
    MaterialStorage() = default;

    void initialize(BufferResourceStorage* bufferStorage, ShaderComponentType* shaderStorage) {
        this->bufferStorage = bufferStorage;
        this->shaderStorage = shaderStorage;
    }

    MaterialKey2 instantiate(std::string_view materialName, size_t capacity = 8);

    void compilePass(MaterialKey2 material, RenderingPassType pass);

    void onFrameFinished(Renderer& renderer);

    MaterialInstanceBuffer2 createInstanceBuffer(const ShaderClass* matClass, const char* defaultParamBlock);
};