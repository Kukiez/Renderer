#pragma once
#include "../Features.h"
#include <Renderer/Resource/Texture/RenderTexture.h>

class RenderTargetSpecific final : public GraphicsFeature {
    RenderTexture texture{};
public:
    RenderTargetSpecific(const RenderTexture fbo) : texture(fbo) {}

    void push(GraphicsContext &renderer) override;

    std::string_view name() const override { return "RenderTargetSpecific"; }
};