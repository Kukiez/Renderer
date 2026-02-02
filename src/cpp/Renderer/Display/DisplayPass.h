#pragma once
#include <Renderer/Graphics/GraphicsPass.h>
#include <Renderer/Resource/Texture/TextureKey.h>

enum class TonemapMethod : int {
    REINHARD,
    RTT,
    FILMIC,
    ACES
};

struct DisplayPassDesc {
    SamplerKey scene{};
    SamplerKey bloom{};
    float bloomStrength{};
    bool isBloomEnabled{};

    struct Tonemap {
        TonemapMethod method{};
        float exposure{};
        bool isEnabled{};
    };
    struct SSAO {
        bool isEnabled{};
        SamplerKey ssao{};
    };
    Tonemap tonemap{};
    SSAO ssao{};
};

class DisplayPass : public IRenderPass {
protected:
    DisplayPassDesc desc;
public:
    DisplayPass(const std::string_view name, const DisplayPassDesc &desc)
    : IRenderPass(name), desc(desc) {}

    void render(GraphicsContext& ctx) const override;
};

class DisplayDepthPass : public IRenderPass {
    SamplerKey depthTexture{};
    Viewport viewport{};
    BufferKey cameraBuffer{};
public:
    DisplayDepthPass(const std::string_view name, SamplerKey sampler, BufferKey cameraBuffer, const Viewport &viewport = {}) : IRenderPass(name), depthTexture(sampler), cameraBuffer(cameraBuffer), viewport(viewport) {}

    void render(GraphicsContext& ctx) const override;
};


