#pragma once
#include <Renderer/Graphics/Passes/IRenderPass.h>
#include <Renderer/Resource/Texture/TextureKey.h>


class BRDFLutBakePass : public IRenderPass {
    TextureKey brdfLUT;
public:
    BRDFLutBakePass(const std::string_view name, TextureKey brdfLUT) : IRenderPass(name), brdfLUT(brdfLUT) {}

    void render(GraphicsContext &ctx) const override;
};
