#include "Cubemap.h"
#include "GLTextureUtils.h"

Cubemap::Cubemap(const int width, const int height, const TextureCreateParams &params): TextureBase(width, height) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    const auto mipmaps = TextureUtils::getMipmapLevels(params.mipmap, width, height);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipmaps, opengl_enum_cast(params.format), width, height);

    TextureUtils::setTexParameters(params, TextureTarget::CUBEMAP);
    TextureUtils::setTexAnisotropy(params, TextureTarget::CUBEMAP);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

Cubemap::Cubemap(const CubemapImage &image, const TextureCreateParams &params): TextureBase(image.width(), image.height()) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    const auto mipmaps = TextureUtils::getMipmapLevels(params.mipmap, image.width(), image.height());

    glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipmaps, opengl_enum_cast(params.format), image.width(), image.height());

    bool generateMipmaps = mipmaps > 1;

    for (const auto face : CubemapFaces) {
        if (const void* pixels = image.getFace(face).pixels()) {
            glTexSubImage2D(opengl_enum_cast(face), 0, 0, 0,
                            image.width(), image.height(), opengl_enum_cast(image.channels()),
                            opengl_enum_cast(image.pixelType()), pixels
            );
        } else {
            generateMipmaps = false;
        }
    }

    TextureUtils::setTexParameters(params, TextureTarget::CUBEMAP);
    TextureUtils::setTexAnisotropy(params, TextureTarget::CUBEMAP);

    if (generateMipmaps) {
        TextureUtils::setTexMipmap(params, TextureTarget::CUBEMAP);
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}
