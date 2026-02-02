#include "ShaderCache.h"
#include "ShaderClass.h"
#include <memory/hash.h>

#include "Shader.h"

ShaderCache::ShaderClassChain::ShaderClassChain(size_t capacity) {
    auto& back = classes.emplace_back();
    back.reserve(capacity);
}

const ShaderClass & ShaderCache::ShaderClassChain::addClass(ShaderClass &&shaderClass) {
    auto* back = &classes.back();

    if (back->size() == back->capacity()) {
        back = &classes.emplace_back();
        back->reserve((back - 1)->capacity() * 2);
    }

    back->emplace_back(std::move(shaderClass));
    return back->back();
}

ShaderCache::ShaderCache() : classes(32) {
    ShaderClass nullClass;
    classes.classes.front().emplace_back(std::move(nullClass));
}

const Shader * ShaderCache::findShader(const std::string_view path) {
    size_t hash = mem::string_hash{}(path);

    const auto it = fileToShader.find(hash);
    if (it != fileToShader.end()) {
        return it->second;
    }
    return nullptr;
}

const Shader& ShaderCache::addShader(const std::string_view path, Shader &&shader) {
    size_t hash = mem::string_hash{}(path);

    auto mem = shaderAllocator.allocate(mem::type_info_of<Shader>, 1);
    new (mem) Shader(std::move(shader));

    fileToShader.emplace(hash, static_cast<Shader*>(mem));
    return *static_cast<Shader*>(mem);
}

const ComputeShader & ShaderCache::addComputeShader(std::string_view path, ComputeShader &&shader) {
    size_t hash = mem::string_hash{}(path);

    auto mem = shaderAllocator.allocate(mem::type_info_of<ComputeShader>, 1);
    new (mem) ComputeShader(std::move(shader));

    fileToShader.emplace(hash, static_cast<ComputeShader*>(mem));
    return *static_cast<ComputeShader*>(mem);
}

ShaderString ShaderCache::internString(std::string_view str) {
    const auto it = stringInternStorage.find(str);

    if (it != stringInternStorage.end()) {
        return it->second;
    }
    auto* mem = static_cast<char*>(charAllocator.allocate(mem::type_info_of<char>, str.length() + 1));
    std::memcpy(mem, str.data(), str.length());
    mem[str.length()] = '\0';

    ShaderString internString(mem, str.length(),
        mem::string_hash{}(std::string_view{mem, str.length()})
    );
    stringInternStorage.emplace(std::string_view(mem, str.length()), internString);
    return internString;
}

bool ShaderCache::isStringIntern(const std::string_view str) const {
    const char* data = str.data();

    return charAllocator.does_pointer_belong_here(data);
}

const ShaderClass & ShaderCache::addShaderClass(ShaderClass &&shaderClass) {
    if (shaderClass.isNull()) {
        return classes.getNullClass();
    }

    auto it = nameToShaderClass.find(shaderClass.name());

    if (it == nameToShaderClass.end()) {
        auto& result = classes.addClass(std::move(shaderClass));
        it = nameToShaderClass.emplace(shaderClass.name(), &result).first;
    } else if (!it->second->isEqual(shaderClass)) {
        std::cout << "Shader class name collision: " << shaderClass.name() << std::endl;

        std::cout << "Existing: " << *it->second << std::endl;
        std::cout << "New: " << shaderClass << std::endl;
        assert(false); // TODO error
    }
    return *it->second;
}

const ShaderClass & ShaderCache::findShaderClass(const ShaderString &str) const {
    const auto it = nameToShaderClass.find(str);

    if (it == nameToShaderClass.end()) {
        return classes.getNullClass();
    }
    return *it->second;
}
