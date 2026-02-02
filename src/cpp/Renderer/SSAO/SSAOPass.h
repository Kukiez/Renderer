#pragma once
#include <Renderer/Graphics/Passes/IRenderPass.h>

class SSAOPass : public IRenderPass {
    SSAOPassDesc desc;
public:
    SSAOPass(std::string_view name, SSAOPassDesc desc) : IRenderPass(name), desc(desc) {}

    void render(GraphicsContext &ctx) const override;
};