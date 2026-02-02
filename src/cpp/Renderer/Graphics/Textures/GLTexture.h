#pragma once
#include "Texture.h"
#include <gl/glew.h>
#include "Image/CubemapImage.h"
#include <util/enum_bit.h>

enum class GLType {
    BYTE = GL_BYTE,
    UNSIGNED_BYTE = GL_UNSIGNED_BYTE,
    SHORT = GL_SHORT,
    UNSIGNED_SHORT = GL_UNSIGNED_SHORT,
    INTEGER = GL_INT,
    UNSIGNED_INT = GL_UNSIGNED_INT,
    FLOAT = GL_FLOAT,
    DOUBLE = GL_DOUBLE,
    HALF_FLOAT = GL_HALF_FLOAT,
    UNSIGNED_332 = GL_UNSIGNED_BYTE_3_3_2,
    UNSIGNED_233R = GL_UNSIGNED_BYTE_2_3_3_REV
};

constexpr GLenum opengl_enum_cast(const TextureCompareMode mode) {
    switch (mode) {
        case TextureCompareMode::NONE: return GL_NONE;
        case TextureCompareMode::REF_TO_TEXTURE: return GL_COMPARE_REF_TO_TEXTURE;
        default: assert(false);
    }
}

constexpr GLenum opengl_enum_cast(const TextureCompareFunc func) {
    static constexpr GLenum funcs[] = {
        GL_LESS,
        GL_LEQUAL,
        GL_GEQUAL,
        GL_GREATER
    };
    return funcs[static_cast<size_t>(func)];
}

constexpr GLenum opengl_enum_cast(const TextureTarget target) {
    switch (target) {
        case TextureTarget::TEXTURE_2D: return GL_TEXTURE_2D;
        case TextureTarget::CUBEMAP: return GL_TEXTURE_CUBE_MAP;
        case TextureTarget::TEXTURE_3D: return GL_TEXTURE_3D;
        case TextureTarget::TEXTURE_1D: return GL_TEXTURE_1D;
        case TextureTarget::TEXTURE_1D_ARRAY: return GL_TEXTURE_1D_ARRAY;
        case TextureTarget::TEXTURE_ARRAY_2D: return GL_TEXTURE_2D_ARRAY;
        case TextureTarget::TEXTURE_2D_MSAA: return GL_TEXTURE_2D_MULTISAMPLE;
        default: assert(false);
    }
}

constexpr GLenum opengl_enum_cast(const CubemapFace face) {
    switch (face) {
        case CubemapFace::POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        case CubemapFace::NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        case CubemapFace::POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        case CubemapFace::NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        case CubemapFace::POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        case CubemapFace::NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        default: assert(false);
    }
}


inline GLenum opengl_enum_cast(const TextureFormat format) {
    switch (format) {
        case TextureFormat::RGBA:             return GL_RGBA;
        case TextureFormat::RGB:              return GL_RGB;
        case TextureFormat::RG:               return GL_RG;
        case TextureFormat::RED:              return GL_RED;
        case TextureFormat::RGBA_4:            return GL_RGBA4;
        case TextureFormat::RGBA_8:            return GL_RGBA8;
        case TextureFormat::RGB_8:             return GL_RGB8;
        case TextureFormat::RG_8:              return GL_RG8;
        case TextureFormat::R_8:               return GL_R8;
        case TextureFormat::RGBA_16F:          return GL_RGBA16F;
        case TextureFormat::RGB_16F:           return GL_RGB16F;
        case TextureFormat::RG_16F:            return GL_RG16F;
        case TextureFormat::R_16F:             return GL_R16F;
        case TextureFormat::RGBA_32F:          return GL_RGBA32F;
        case TextureFormat::RGB_32F:           return GL_RGB32F;
        case TextureFormat::RG_32F:            return GL_RG32F;
        case TextureFormat::R_32F:             return GL_R32F;
        case TextureFormat::R_11F_G11F_B10F:   return GL_R11F_G11F_B10F;
        case TextureFormat::RGBA8_SNORM:      return GL_RGBA8_SNORM;
        case TextureFormat::RGB8_SNORM:       return GL_RGB8_SNORM;
        case TextureFormat::RGBA_16:           return GL_RGBA16;
        case TextureFormat::RGB_16:            return GL_RGB16;
        case TextureFormat::DEPTH24_STENCIL8: return GL_DEPTH24_STENCIL8;
        case TextureFormat::DEPTH32F_STENCIL8: return GL_DEPTH32F_STENCIL8;
        case TextureFormat::DEPTH_16F: return GL_DEPTH_COMPONENT16;
        case TextureFormat::DEPTH_32F: return GL_DEPTH_COMPONENT32;
        default: assert(false);
    }
}

constexpr GLenum opengl_enum_cast(const ImageChannels ch) {
    switch (ch) {
        case ImageChannels::R: return GL_RED;
        case ImageChannels::RG: return GL_RG;
        case ImageChannels::RGB: return GL_RGB;
        case ImageChannels::RGBA: return GL_RGBA;
        default: assert(false);
    }
}


constexpr GLenum opengl_enum_cast(const PassBarrier barrier) {
    GLenum result = GL_NONE;

    if ((barrier & PassBarrier::SHADER_STORAGE) == PassBarrier::SHADER_STORAGE) {
        result |= GL_SHADER_STORAGE_BARRIER_BIT;
    }
    if ((barrier & PassBarrier::COMMAND) == PassBarrier::COMMAND) {
        result |= GL_COMMAND_BARRIER_BIT;
    }
    if ((barrier & PassBarrier::TEXTURE_FETCH) == PassBarrier::TEXTURE_FETCH) {
        result |= GL_TEXTURE_FETCH_BARRIER_BIT;
    }
    return result;
};

constexpr GLenum opengl_enum_cast(const PixelType type) {
    switch (type) {
        case PixelType::BYTE: return GL_BYTE;
        case PixelType::UNSIGNED_BYTE: return GL_UNSIGNED_BYTE;
        case PixelType::SHORT: return GL_SHORT;
        case PixelType::UNSIGNED_SHORT: return GL_UNSIGNED_SHORT;
        case PixelType::INTEGER: return GL_INT;
        case PixelType::UNSIGNED_INT: return GL_UNSIGNED_INT;
        case PixelType::HALF_FLOAT: return GL_HALF_FLOAT;
        case PixelType::FLOAT: return GL_FLOAT;
        case PixelType::DOUBLE: return GL_DOUBLE;
        default: assert(false);
    }
}

constexpr GLenum opengl_enum_cast(const TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::REPEAT: return GL_REPEAT;
        case TextureWrap::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
        case TextureWrap::EDGE_CLAMP: return GL_CLAMP_TO_EDGE;
        case TextureWrap::BORDER_CLAMP: return GL_CLAMP_TO_BORDER;
        default: assert(false);
    }
}

constexpr GLenum opengl_enum_cast(const TextureMinFilter filter) {
    switch (filter) {
        case TextureMinFilter::NEAREST: return GL_NEAREST;
        case TextureMinFilter::LINEAR:  return GL_LINEAR;
        case TextureMinFilter::NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
        case TextureMinFilter::LINEAR_MIPMAP_NEAREST:  return GL_LINEAR_MIPMAP_NEAREST;
        case TextureMinFilter::NEAREST_MIPMAP_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
        case TextureMinFilter::LINEAR_MIPMAP_LINEAR:  return GL_LINEAR_MIPMAP_LINEAR;
        default: assert(false);
    }
}

constexpr GLenum opengl_enum_cast(const TextureMagFilter filter) {
    switch (filter) {
        case TextureMagFilter::NEAREST: return GL_NEAREST;
        case TextureMagFilter::LINEAR:  return GL_LINEAR;
        default: assert(false);
    }
}