#pragma once
#include <Renderer/RenderingStages/LoadPass.h>
#include <Renderer/Resource/Geometry/GeometryKey.h>
#include <Renderer/Resource/Shader/ShaderKey.h>
#include <Renderer/Resource/Texture/TextureKey.h>

class Renderer;

class MainRenderInvocation {
    TextureKey depthTexture;
    TextureKey colorTexture;

    TextureKey finalColor_RGB8;
    TextureKey rgba16fPingpong;

    GeometryKey quadGeometry;

    int frameIndex = 0;
public:
    void onLoad(RendererLoadView view);

    void onRender(Renderer& renderer) const;

    TextureKey getDepthTexture() const {
        return depthTexture;
    }

    TextureKey getColorTexture() const {
        return colorTexture;
    }

    TextureKey getFinalColorTexture() const {
        return finalColor_RGB8;
    }

    TextureKey getRgba16fPingpong() const {
        return rgba16fPingpong;
    }
};
