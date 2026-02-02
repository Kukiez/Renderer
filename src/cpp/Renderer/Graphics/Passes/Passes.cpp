#include <openGL/Shader/ShaderCompiler.h>
#include <Renderer/Graphics/GraphicsContext.h>

#include "ClearPass.h"


void ClearPass::render(GraphicsContext &ctx) const {
    auto old = ctx.bindTextureForRendering(texture);

    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClearDepth(clearColor.r);

    if (texture.isColorAttachment()) {
        glClear(GL_COLOR_BUFFER_BIT);
    } else if (texture.isDepthAttachment()) {
        glClear(GL_DEPTH_BUFFER_BIT);
    } else if (texture.isStencilAttachment()) {
        glClear(GL_STENCIL_BUFFER_BIT);
    } else if (texture.isDepthStencilAttachment()) {
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
}