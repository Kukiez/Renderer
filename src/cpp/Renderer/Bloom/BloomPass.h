#pragma once
#include <Renderer/Graphics/Passes/IRenderPass.h>
#include <Renderer/Resource/Texture/TextureKey.h>

struct BloomParams {
    float threshold = 1.f;
    int passes = 7;
    float filterRadius = 0.005f;
};

class BloomPass : public IRenderPass {
    TextureKey inBloomTexture;
    TextureKey outBloomResult{};

    BloomParams bloom{};
public:
    BloomPass(std::string_view name, TextureKey inBloomTexture, TextureKey outBloomResult, BloomParams params)
    : IRenderPass(name), inBloomTexture(inBloomTexture), outBloomResult(outBloomResult), bloom(params) {}

    void render(GraphicsContext &ctx) const override;
};