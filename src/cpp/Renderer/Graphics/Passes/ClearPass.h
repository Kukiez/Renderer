#pragma once
#include <Renderer/Resource/Texture/RenderTexture.h>

#include "IRenderPass.h"

class ClearPass final : public IRenderPass {
    RenderTexture texture{};
    glm::vec4 clearColor{};
    ClearTarget clearTargets{};
public:
    explicit ClearPass(const std::string_view name, const RenderTexture &texture, const glm::vec4 clearColor, ClearTarget clearTargets = {})
    : IRenderPass(name), clearColor(clearColor), texture(texture), clearTargets(clearTargets) {}

    RENDERERAPI void render(GraphicsContext& ctx) const override;
};
