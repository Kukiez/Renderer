#pragma once
#include <string_view>
#include <unordered_map>
#include <memory/byte_arena.h>

#include "ShaderReflection.h"

class ComputeShader;
class ShaderClass;
class Shader;

class ShaderCache {
    struct ShaderClassChain {
        std::vector<std::vector<ShaderClass>> classes;

        explicit ShaderClassChain(size_t capacity);

        const ShaderClass& addClass(ShaderClass&& shaderClass);

        const ShaderClass& getNullClass() const {
            return classes.front().front();
        }
    };

    ShaderClassChain classes;

    mem::byte_arena<> shaderAllocator = mem::byte_arena<>(0.01 * 1024 * 1024);
    mem::byte_arena<> charAllocator = mem::byte_arena<>(0.01 * 1024 * 1024);

    std::unordered_map<size_t, const Shader*> fileToShader;
    std::unordered_map<std::string_view, ShaderString> stringInternStorage;

    std::unordered_map<ShaderString, const ShaderClass*> nameToShaderClass{};
public:
    ShaderCache();

    const Shader* findShader(std::string_view path);

    const Shader& addShader(std::string_view path, Shader&& shader);
    const ComputeShader& addComputeShader(std::string_view path, ComputeShader&& shader);

    ShaderString internString(std::string_view str);

    bool isStringIntern(std::string_view str) const;

    const ShaderClass& addShaderClass(ShaderClass&& shaderClass);

    const ShaderClass& getNullShaderClass() const { return classes.getNullClass(); }

    const ShaderClass& findShaderClass(const ShaderString& str) const;
};
