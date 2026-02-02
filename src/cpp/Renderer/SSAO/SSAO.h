#pragma once
#include <Renderer/Resource/Buffer/BufferKey.h>
#include <Renderer/Resource/Texture/TextureKey.h>

class SSAOSettings {
    friend class SSAOPass;
    friend class SSAOResources;

    BufferKey kernelBuffer;
    TextureKey noiseTexture;
    int kernelSize = 64;
    int noiseResolution = 16;
public:
    float radius = 0.5f;
    float bias = 0.025f;
};


struct SSAOPassDesc {
    SamplerKey inDepthTexture;
    TextureKey outSSAONoise;
    TextureKey outSSAOResult;

    BufferKey cameraBuffer;

    SSAOSettings settings;
};