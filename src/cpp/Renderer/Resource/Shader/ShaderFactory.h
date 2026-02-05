#pragma once
#include "ShaderDescriptor.h"
#include "ShaderKey.h"
#include "RendererAPI.h"

class ShaderComponentType;
class Renderer;

class RENDERERAPI ShaderFactory {
    friend class ShaderSynchronousFactory;

    ShaderComponentType* type;
public:
    explicit ShaderFactory(Renderer& renderer);

    ShaderFactory(ShaderComponentType* type) : type(type) {}

    ShaderKey loadShader(const ShaderDescriptor &descriptor) const;
    ShaderKey loadComputeShader(const ComputeShaderDescriptor &descriptor) const;
};