#pragma once

class RENDERERAPI PrefilterMapPass final : public EnvCubemapPass {
    UniformParameterIndex roughnessIndex{};
    Mipmap mipmaps{};
public:
    using EnvCubemapPass::EnvCubemapPass;

    void setMips(Mipmap levels) {
        mipmaps = levels;
    }

    void setRoughness(std::string_view rough);

    void render(GraphicsContext& ctx) const override;
};
