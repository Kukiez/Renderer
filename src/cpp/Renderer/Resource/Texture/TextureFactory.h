#pragma once
#include "TextureDescriptor.h"
#include "TextureKey.h"

class TextureResourceType;
class Renderer;

class RENDERERAPI TextureFactory {
    TextureResourceType* textures;
public:
    TextureFactory(TextureResourceType* textureStorage) : textures(textureStorage) {}
    TextureFactory(Renderer& renderer);

    TextureKey createTexture2D(TextureDescriptor2D& descriptor) const;
    TextureKey createTexture2D(TextureDescriptor2D&& descriptor) const;

    TextureKey createCubemap(const EmptyCubemapDescriptor2D& descriptor) const;

    TextureKey createCubemap(const TextureCreateParams& params, const int width, const int height) const {
        return createCubemap({params, width, height});
    }

    TextureKey createTexture2DMS(const TextureDescriptor2DMS& descriptor) const;

    TextureKey createTextureArray2D(TextureDescriptor2DArray&& descriptor) const;

    TextureKey createTextureArray2D(TextureDescriptor2DArray& descriptor) const {
        return createTextureArray2D(std::move(descriptor));
    }
};