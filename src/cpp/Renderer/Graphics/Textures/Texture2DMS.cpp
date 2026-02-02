#include "Texture2DMS.h"

#include <gl/glew.h>

#include "GLTexture.h"

Texture2DMS::Texture2DMS(int width, int height, int samples, TextureFormat format) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);

    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, opengl_enum_cast(format), width, height, GL_TRUE);
}
