#pragma once
#include <Filesystem/Filepath.h>
#include <Renderer/Graphics/Passes/IRenderPass.h>
#include <Renderer/Skybox/SkyboxSystem.h>

class RENDERERAPI SkyboxBakePass : public IRenderPass {
    Skybox* outSkybox{};
    Filepath hdriFile;
public:
    SkyboxBakePass(const std::string_view name, Skybox* outSkybox, const Filepath& hdriFile)
        : IRenderPass(name), outSkybox(outSkybox), hdriFile(hdriFile) {}

    void render(GraphicsContext& ctx) const override;
};
