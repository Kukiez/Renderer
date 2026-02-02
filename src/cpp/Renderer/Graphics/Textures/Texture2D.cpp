#pragma once
#include "Texture2D.h"

#include <gl/glew.h>
#include <Image/Image.h>
#include <Image/ImageArray.h>

#include "GLTextureUtils.h"
#include "GLTexture.h"

Texture2D::Texture2D(const int width, const int height, const TextureCreateParams &params): TextureBase(width, height) {
    gen(TextureTarget::TEXTURE_2D);

    const int mipmapLevels = TextureUtils::getMipmapLevels(params.mipmap, width, height);
    glTexStorage2D(GL_TEXTURE_2D, mipmapLevels, opengl_enum_cast(params.format), myWidth, myHeight);

    TextureUtils::setTexParameters(params, TextureTarget::TEXTURE_2D);
    TextureUtils::setTexAnisotropy(params, TextureTarget::TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D::Texture2D(const Image &image, const TextureCreateParams &params) : TextureBase(image.width(), image.height()) {
    gen(TextureTarget::TEXTURE_2D);

    const int mipmapLevels = TextureUtils::getMipmapLevels(params.mipmap, image.width(), image.height());

    glTexStorage2D(GL_TEXTURE_2D, mipmapLevels, opengl_enum_cast(params.format), myWidth, myHeight);

    TextureUtils::setTexParameters(params, TextureTarget::TEXTURE_2D);


    // switch (image.channels()) {
    //     case ImageChannels::R: glPixelStorei(GL_UNPACK_ALIGNMENT, 1); break;
    //     case ImageChannels::RG: glPixelStorei(GL_UNPACK_ALIGNMENT, 2); break;
    //     case ImageChannels::RGB: glPixelStorei(GL_UNPACK_ALIGNMENT, 4); break;
    //     case ImageChannels::RGBA: glPixelStorei(GL_UNPACK_ALIGNMENT, 4); break;
    // }

    if (image.pixels()) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, myWidth, myHeight,
            opengl_enum_cast(image.channels()), opengl_enum_cast(image.pixelType()), image.pixels()
        );

        if (mipmapLevels > 1) {
            TextureUtils::setTexMipmap(params, TextureTarget::TEXTURE_2D);
        }
    }
    TextureUtils::setTexAnisotropy(params, TextureTarget::TEXTURE_2D);

    if (params.wrap == TextureWrap::BORDER_CLAMP) {
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, reinterpret_cast<const float *>(&params.border.color));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2DArray::Texture2DArray(int width, int height, int depth, const TextureCreateParams &params) {
    gen(TextureTarget::TEXTURE_ARRAY_2D);

    const int mipmapLevels = TextureUtils::getMipmapLevels(params.mipmap, width, height);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipmapLevels, opengl_enum_cast(params.format), width, height, depth);
    TextureUtils::setTexParameters(params, TextureTarget::TEXTURE_ARRAY_2D);
    TextureUtils::setTexAnisotropy(params, TextureTarget::TEXTURE_ARRAY_2D);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

Texture2DArray::Texture2DArray(const ImageArray &imageArray, const TextureCreateParams &params) {
    gen(TextureTarget::TEXTURE_ARRAY_2D);

    const int mipLevels = TextureUtils::getMipmapLevels(params.mipmap, imageArray.width(), imageArray.height());
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevels, opengl_enum_cast(params.format), imageArray.width(), imageArray.height(), imageArray.depth());

    if (imageArray.hasData()) {
        for (int i = 0; i < imageArray.depth(); i++) {
            ImageArray::Slice slice = imageArray[i];

            if (slice.pixels == nullptr) continue;

            glTexSubImage3D(
                GL_TEXTURE_2D_ARRAY, 0, slice.xOffset, slice.yOffset, i, slice.width, slice.height, 1,
                opengl_enum_cast(imageArray.channels()), opengl_enum_cast(imageArray.pixelType()), slice.pixels
            );
        }

        if (mipLevels > 1) {
            TextureUtils::setTexMipmap(params, TextureTarget::TEXTURE_ARRAY_2D);
        }
    }

    TextureUtils::setTexParameters(params, TextureTarget::TEXTURE_ARRAY_2D);
    TextureUtils::setTexAnisotropy(params, TextureTarget::TEXTURE_ARRAY_2D);

    if (params.wrap == TextureWrap::BORDER_CLAMP) {
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, reinterpret_cast<const float *>(&params.border.color));
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
