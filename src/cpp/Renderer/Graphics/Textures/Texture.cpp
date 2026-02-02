#include "Texture.h"
#include <ostream>
#include <gl/glew.h>
#include "GLTextureUtils.h"
#include "GLTexture.h"


std::ostream & operator<<(std::ostream &os, const TextureTarget &target) {
    switch (target) {
        case TextureTarget::TEXTURE_1D:
            os << "TEXTURE_1D"; break;
        case TextureTarget::TEXTURE_1D_ARRAY:
            os << "TEXTURE_1D_ARRAY"; break;
        case TextureTarget::TEXTURE_2D:
            os << "TEXTURE_2D"; break;
        case TextureTarget::TEXTURE_ARRAY_2D:
            os << "TEXTURE_ARRAY_2D"; break;
        case TextureTarget::CUBEMAP:
            os << "CUBEMAP"; break;
        case TextureTarget::TEXTURE_3D:
            os << "TEXTURE_3D"; break;
        default:
            os << "UNKNOWN_TEXTURE_TARGET"; break;
    }
    return os;
}

void TextureBase::gen(const TextureTarget target) {
    if (texture) {
        glDeleteTextures(1, &texture);
    }
    glGenTextures(1, &texture);
    glBindTexture(opengl_enum_cast(target), texture);
}

void TextureBase::discard() {
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
        myWidth = 0;
        myHeight = 0;
        myDepth = 0;
    }
}
