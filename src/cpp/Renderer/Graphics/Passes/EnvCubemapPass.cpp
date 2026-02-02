#include "EnvCubemapPass.h"

#include <openGL/Shader/ShaderCompiler.h>
#include <openGL/Shader/ShaderReflection.h>
#include <Renderer/Graphics/GraphicsContext.h>
#include <Renderer/Graphics/PushConstant.h>
#include <Renderer/Graphics/DrawContainers/StandardDrawContainer.h>
#include <Renderer/Resource/Geometry/GeometryQuery.h>
#include <Renderer/Resource/Texture/RenderTexture.h>
#include <Renderer/Skybox/SkyboxSystem.h>

#include "PrefilterPass.h"

auto EnvCubemapPass::setViews(std::string_view name, const glm::mat4 *views) -> void {
    memcpy(this->views, views, 6 * sizeof(glm::mat4));
    viewIndex = shader->getUniformParameter(ShaderString::fromString(name));
}

void EnvCubemapPass::render(GraphicsContext &ctx) const {
    if (viewIndex == UniformParameterIndex::INVALID) return;

    const ShaderUniformParameter& viewParameter = shader->definition().parameters()[viewIndex];

    bindPass(ctx);

    GeometryQuery gq(*ctx.getRenderer());

    auto& skyboxResources = ctx.getRenderer()->getResource<SkyboxRenderer>();
    auto geometry = skyboxResources.cubemapGeometryKey;

    for (auto face : CubemapFaces) {
        DrawCommand cmd;
        cmd.geometry = geometry;
        cmd.drawRange = gq.getFullDrawRange(cmd.geometry);

        ctx.bindTextureForRendering(RenderTexture(targetCubemap, face));

        PushConstantSet::bind(ctx, shader, viewParameter, &views[static_cast<int>(face)]);
        StandardDrawContainer::draw(ctx, &cmd, 1);
    }
}

void PrefilterMapPass::setRoughness(std::string_view rough) {
    roughnessIndex = shader->getUniformParameter(cexpr::type_hash(rough.data()));
}

void PrefilterMapPass::render(GraphicsContext &ctx) const {
    if (viewIndex == UniformParameterIndex::INVALID || roughnessIndex == UniformParameterIndex::INVALID) return;

    const ShaderUniformParameter& viewParameter = shader->definition().parameters()[viewIndex];
    const ShaderUniformParameter& roughnessParameter = shader->definition().parameters()[roughnessIndex];

    bindPass(ctx);

    GeometryQuery gq(*ctx.getRenderer());

    auto& skyboxResources = ctx.getRenderer()->getResource<SkyboxRenderer>();
    auto geometry = skyboxResources.cubemapGeometryKey;

    uint8_t mips = static_cast<uint8_t>(mipmaps);

    for (uint8_t i = 0; i < mips; ++i) {
        const float roughness = static_cast<float>(i) / static_cast<float>(mips - 1);

        PushConstantSet::bind(ctx, shader, roughnessParameter, &roughness);

        for (auto face : CubemapFaces) {
            DrawCommand cmd;
            cmd.geometry = geometry;
            cmd.drawRange = gq.getFullDrawRange(cmd.geometry);

            ctx.bindTextureForRendering(RenderTexture(targetCubemap, face, Mipmap{i}));
            PushConstantSet::bind(ctx, shader, viewParameter, &views[static_cast<int>(face)]);

            StandardDrawContainer::draw(ctx, &cmd, 1);
        }
    }
}
