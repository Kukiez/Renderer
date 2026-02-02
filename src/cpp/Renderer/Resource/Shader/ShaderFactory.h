#pragma once
#include "ShaderDescriptor.h"
#include "ShaderKey.h"

class ShaderComponentType;
class Renderer;

class ShaderFactory {
    friend class ShaderSynchronousFactory;

    static void setNamedShader(ShaderComponentType* shaders, ShaderKey key, ComponentIndex name, const mem::type_info* type);

    ShaderComponentType* type;
public:
    explicit ShaderFactory(Renderer& renderer);

    ShaderFactory(ShaderComponentType* type) : type(type) {}

    ShaderKey loadShader(const ShaderDescriptor &descriptor) const;
    ShaderKey loadComputeShader(const ComputeShaderDescriptor &descriptor) const;

    template <typename Shader>
    void setNamedShader(ShaderKey key) const {
        setNamedShader(type, key, ComponentTypeRegistry::getComponentIndex<Shader>(), mem::type_info_of<Shader>);
    }
};