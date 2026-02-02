#pragma once
#include <Renderer/Resource/Shader/ShaderKey.h>

class Renderer;

class SSAOResources {
    ShaderKey ssaoShader;
    ShaderKey ssaoBlurShader;
public:
    void onLoad(RendererLoadView renderer);

    static SSAOSettings createSSAOSettings(Renderer& renderer, int kernelSize, int noiseResolution);

    ShaderKey getSSAOShader() const { return ssaoShader; }
    ShaderKey getSSAOBlurShader() const { return ssaoBlurShader; }
};