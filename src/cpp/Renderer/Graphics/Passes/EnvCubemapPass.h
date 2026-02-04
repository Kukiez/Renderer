#pragma once
#include <Renderer/Graphics/GraphicsPass.h>
#include <glm/glm.hpp>
#include <Renderer/Resource/Texture/TextureKey.h>

class EnvCubemapPass : public IGraphicsPass {
protected:
    glm::mat4 views[6];
    TextureKey targetCubemap;
    UniformParameterIndex viewIndex;
    ClearTarget clearTargets[6]{};
public:
    using IGraphicsPass::IGraphicsPass;

    void setTargetCubemap(const TextureKey texture) { targetCubemap = texture; }

    RENDERERAPI void setViews(std::string_view name, const glm::mat4 *views);

    void setClearTargets(ClearTarget targetForAll) {
        for (auto& t : clearTargets) t = targetForAll;
    }

    void setClearTargets(ClearTarget targets[6]) {
        memcpy(clearTargets, targets, 6 * sizeof(ClearTarget));
    }

    void setCaptureProjection(std::string_view name, const glm::mat4& projection) {
        pushConstantsBlock.push(name, projection);
    }

    void setInputTexture(std::string_view name, const TextureKey texture) {
        pushConstantsBlock.push(name, texture);
    }

    void render(GraphicsContext& ctx) const override;
};
