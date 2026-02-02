#pragma once
#include "IRenderPass.h"
#include "Renderer/Graphics/State/RenderState.h"
#include "Renderer/Resource/Texture/RenderTexture.h"
#include "Renderer/Resource/Texture/TextureKey.h"


class FullScreenPass : public IRenderPass {
    RenderTexture outTexture;
    SamplerKey inTexture;
    Viewport vp{};
public:
    FullScreenPass(const std::string_view name, const RenderTexture& outTexture, const SamplerKey& inTexture, const Viewport& vp) : IRenderPass(name), outTexture(outTexture), inTexture(inTexture), vp(vp) {}

    void render(GraphicsContext& ctx) const override;
};
