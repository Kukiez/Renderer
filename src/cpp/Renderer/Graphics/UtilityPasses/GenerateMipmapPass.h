#pragma once
#include <Renderer/Graphics/Features.h>
#include <Renderer/Resource/Texture/TextureKey.h>

#include "../Passes/IRenderPass.h"

class GenerateMipmapPass final : public IRenderPass {
    TextureKey texture{};
public:
    explicit GenerateMipmapPass(std::string_view name, TextureKey texture)
    : IRenderPass(name), texture(texture) {}

    void render(GraphicsContext& ctx) const override;
};
