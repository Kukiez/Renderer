#pragma once
#include <Renderer/Graphics/Passes/IRenderPass.h>
#include <Renderer/Resource/Texture/RenderTexture.h>
#include <Renderer/Skybox/SkyboxSystem.h>

class SkyboxPass : public IRenderPass {
    RenderTexture colorTarget{};
    RenderTexture depthTarget{};
    BufferKey view{};
    SamplerKey skybox{};
public:
    SkyboxPass(const std::string_view name,
        const RenderTexture &colorTarget,
        const RenderTexture &depthTarget,
        BufferKey view,
        const SamplerKey skybox
     ) : IRenderPass(name), colorTarget(colorTarget), depthTarget(depthTarget), view(view), skybox(skybox) {}

    void render(GraphicsContext& ctx) const override;
};