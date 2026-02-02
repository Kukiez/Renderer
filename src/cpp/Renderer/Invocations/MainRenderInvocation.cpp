#include "MainRenderInvocation.h"

#include <Other/AABBRenderer.h>
#include <Renderer/Resource/Shader/ShaderFactory.h>
#include <Renderer/Resource/Texture/TextureFactory.h>

void MainRenderInvocation::onLoad(RendererLoadView view) {
    const TextureFactory tf(view.getRenderer());
    const ShaderFactory sf(view.getRenderer());

    auto preset = TexturePreset::SHADOW_2D_32F;

    depthTexture = tf.createTexture2D(TextureDescriptor2D(preset, 2560, 1440));

    TextureCreateParams mainColorQuality;
    mainColorQuality.format = TextureFormat::RGBA_16F;
    mainColorQuality.minFilter = TextureMinFilter::LINEAR;
    mainColorQuality.magFilter = TextureMagFilter::LINEAR;
    mainColorQuality.wrap = TextureWrap::EDGE_CLAMP;

    colorTexture = tf.createTexture2D(TextureDescriptor2D(mainColorQuality, 2560, 1440));
    rgba16fPingpong = tf.createTexture2D(TextureDescriptor2D(mainColorQuality, 2560, 1440));

    TextureCreateParams finalColorParams;
    finalColorParams.format = TextureFormat::RGB_8;
    finalColorParams.minFilter = TextureMinFilter::LINEAR;
    finalColorParams.magFilter = TextureMagFilter::LINEAR;
    finalColorParams.wrap = TextureWrap::EDGE_CLAMP;
    finalColor_RGB8 = tf.createTexture2D(TextureDescriptor2D(finalColorParams, 2560, 1440));

    static_assert(sizeof(TextureCreateParams) == 40);
}
