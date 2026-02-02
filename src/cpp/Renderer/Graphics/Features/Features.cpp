#include <Renderer/Graphics/GraphicsContext.h>

#include "RenderStateSpecific.h"
#include "FramebufferIndexedBlend.h"
#include "FramebufferIndexedClear.h"
#include "RenderTargetSpecific.h"

void RenderStateSpecific::push(GraphicsContext &renderer) {
    oldState = renderer.getCurrentState();
    renderer.setCurrentState(state);
}

void RenderTargetSpecific::push(GraphicsContext &ctx) {
    ctx.bindTextureForRendering(texture);
}