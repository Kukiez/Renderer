#include <Renderer/Graphics/GraphicsContext.h>
#include <Renderer/Graphics/Textures/Cubemap.h>
#include <Renderer/Resource/Texture/TextureQuery.h>

#include "GenerateMipmapPass.h"
#include <Renderer/Graphics/Textures/GLTexture.h>
#include <Renderer/Graphics/Textures/Texture2D.h>

void GenerateMipmapPass::render(GraphicsContext &ctx) const {
    TextureQuery q(*ctx.getRenderer());

    GLenum type = opengl_enum_cast(texture.type());

    switch (texture.type()) {
        case TextureTarget::TEXTURE_2D: {
            glBindTexture(type, q.getTexture2D(texture).id());
            glGenerateMipmap(type);
            break;
        }
        case TextureTarget::CUBEMAP: {
            glBindTexture(type, q.getCubemap(texture).id());
            glGenerateMipmap(type);
            break;
        }
        default: assert(false);
    }
}
