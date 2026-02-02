#pragma once

#include "TextureKey.h"
#include <Renderer/Resource/Buffer/BufferKey.h>

class TextureResourceType;

class FrameBufferObject;

class Cubemap;
class Texture2D;
class Texture2DMS;
class TextureAsset;
class Renderer;
struct RenderTexture;
struct MultiRenderTexture;

class TextureQuery {
    TextureResourceType* textures;
public:
    TextureQuery(TextureResourceType* textures) : textures(textures) {}
    TextureQuery(TextureResourceType& textures) : textures(&textures) {}
    TextureQuery(Renderer& renderer);

    const Texture2D& getTexture2D(TextureKey texture) const;
    const Cubemap& getCubemap(TextureKey texture) const;
    const Texture2DMS& getTexture2DMS(TextureKey texture) const;

    BufferKey getMaterial2DBuffer() const;

    FrameBufferObject& getFrameBuffer(const RenderTexture& texture) const;
    FrameBufferObject& getFramebuffer(const MultiRenderTexture& texture) const;
    size_t getTextureGPUHandle(const TextureKey texture) const;

    bool isValid(TextureKey texture) const;
    bool isValid(SamplerKey sampler) const;

    size_t getTextureGPUHandle(SamplerKey sampler) const;
};
