#pragma once
#include "GLTexture.h"
#include "Texture.h"

struct TextureUtils {
    static void setTexParameters(const TextureCreateParams& params, TextureTarget type) {
        const GLenum target = opengl_enum_cast(type);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, opengl_enum_cast(params.minFilter));
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, opengl_enum_cast(params.magFilter));
        glTexParameteri(target, GL_TEXTURE_WRAP_S, opengl_enum_cast(params.wrap));
        glTexParameteri(target, GL_TEXTURE_WRAP_T, opengl_enum_cast(params.wrap));

        if (params.compareMode != TextureCompareMode::NONE) {
            glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, opengl_enum_cast(params.compareMode));
            glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, opengl_enum_cast(params.compareFunc));
        }
    }

    static void setTexAnisotropy(const TextureCreateParams& params, TextureTarget type) {
        const GLenum target = opengl_enum_cast(type);
        if (params.anisotropy == Anisotropy::AUTO) {
            GLfloat maxAniso = 0.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
            glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        }
    }

    static int getMipmapLevels(Mipmap mipmap, const int width, const int height) {
        int mipmapLevels;

        switch (mipmap) {
            case Mipmap::NONE:
                mipmapLevels = 1;
                break;
            case Mipmap::AUTO:
                mipmapLevels = static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;
                break;
            default:
                const auto mip = static_cast<int>(mipmap);
                mipmapLevels = std::max(1, mip);
        }
        return mipmapLevels;
    }

    static void setTexMipmap(const TextureCreateParams& params, TextureTarget target) {
        glGenerateMipmap(opengl_enum_cast(target));
        if (params.lodBias != 0.f) glTexParameterf(opengl_enum_cast(target), GL_TEXTURE_LOD_BIAS, params.lodBias);
    }
};

