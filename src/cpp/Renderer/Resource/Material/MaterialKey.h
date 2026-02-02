#pragma once
#include <Renderer/Resource/Buffer/BufferKey.h>
#include <Renderer/Resource/Shader/ShaderKey.h>

#include "MaterialCooking.h"

class MaterialStorage;
class MaterialInstanceKey2;

class MaterialInstanceBuffer2 {
    friend struct MaterialBuffer;

    void setParameter(ShaderClassMemberIndex paramIdx, const void* value);

    const ShaderClass* materialClass{};
    char* memory{};
public:
    MaterialInstanceBuffer2(const ShaderClass* materialClass, char* memory) : materialClass(materialClass), memory(memory) {}

    template <typename T>
    void setParameter(std::string_view parameter, T&& value) {
        auto idx = materialClass->findMemberIndex(parameter);
        if (idx == ShaderClassMemberIndex::INVALID) return;

        setParameter(idx, &value);
    }

    char* getMemory() { return memory; }
};

struct MaterialBuffer {
    const MaterialDefinition* def{};
    MaterialStorage* storage{};
    BufferKey buffer;
    size_t nextInstanceID = 0;
    std::vector<std::pair<RenderingPassType, ShaderKey>> compiledPasses;

    size_t createInstance(const MaterialInstanceBuffer2& matInst);
};

class MaterialKey2 {
    friend class MaterialInstanceKey2;
    friend class MaterialStorage;
    MaterialBuffer* buffer{};
public:
    MaterialKey2() = default;
    MaterialKey2(MaterialBuffer* buffer) : buffer(buffer) {}

    operator bool() const { return buffer != nullptr; }
    bool operator == (const MaterialKey2& other) const { return buffer == other.buffer; }
    bool operator != (const MaterialKey2& other) const { return !(*this == other); }

    void compile(RenderingPassType pass);

    const MaterialDefinition* getDefinition() const { return buffer->def; }

    ShaderKey getShaderPermutation(RenderingPassType pass) const;

    ShaderKey getOrCreateShaderPermutation(RenderingPassType pass) const;

    bool operator < (const MaterialKey2& other) const { return buffer < other.buffer; }

    MaterialBuffer* getBuffer() const { return buffer; }

    BufferKey getMaterialBuffer() const { return buffer->buffer; }

    MaterialInstanceKey2 createInstance(const MaterialInstanceBuffer2& matInstBuffer);

    MaterialInstanceBuffer2 createInstanceBuffer() const;
};

struct MaterialKeyHash {
    size_t operator () (const MaterialKey2 key) const noexcept {
        return std::hash<void*>{}(key.getBuffer());
    }
};

template <> struct std::hash<MaterialKey2> {
    size_t operator () (const MaterialKey2 key) const noexcept {
        return MaterialKeyHash{}(key);
    }
};

class MaterialInstanceKey2 {
    MaterialKey2 material;
    size_t instanceIndex = 0;
public:
    MaterialInstanceKey2() = default;
    MaterialInstanceKey2(MaterialKey2 material, size_t instanceIndex) : material(material), instanceIndex(instanceIndex) {}

    size_t instanceID() const { return instanceIndex; }

    MaterialKey2 getMaterial() const { return material; }
};