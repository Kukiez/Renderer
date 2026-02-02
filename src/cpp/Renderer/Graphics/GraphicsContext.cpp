#pragma once
#include "GraphicsContext.h"
#include <openGL/Shader/ShaderCompiler.h>
#include <Renderer/Renderer.h>
#include <Renderer/Resource/Geometry/Geometry.h>
#include <Renderer/Resource/Geometry/GeometryQuery.h>
#include <Renderer/Resource/Texture/RenderTexture.h>
#include <Renderer/Resource/Texture/TextureQuery.h>

#include "GraphicsPass.h"
#include <Renderer/Graphics/State/GlobalRenderState.h>

const Geometry * GraphicsContext::getGeometry(const GeometryKey &key) const {
    GeometryQuery q(*renderer);
    return &q.getGeometry(key);
}

void GraphicsContext::setCurrentState(const RenderState &state) {
    currentState = state;
    OPENGL_STATE_APPLICATION.change(currentState);
}

FrameBufferObject* GraphicsContext::bindTextureForRendering(const RenderTexture &texture) {
    TextureQuery q(*renderer);

    auto oldFbo = currentFrameBuffer;

    if (texture.texture == DEFAULT_FRAMEBUFFER_SCREEN_TEXTURE_KEY) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, 2560, 1440);
        currentFrameBuffer = nullptr;
        return oldFbo;
    }
    currentFrameBuffer = &q.getFrameBuffer(texture);
    currentFrameBuffer->bind();
    return oldFbo;
}

FrameBufferObject * GraphicsContext::bindTextureForRendering(const MultiRenderTexture &texture) {
    TextureQuery q(*renderer);

    if (texture.empty()) return nullptr;
    if (texture.size() == 1) return bindTextureForRendering(texture[0]);

    auto fbo = &q.getFramebuffer(texture);

    if (fbo == currentFrameBuffer) return fbo;
    auto oldFbo = currentFrameBuffer;
    currentFrameBuffer = fbo;
    currentFrameBuffer->bind();
    return oldFbo;
}

FrameBufferObject * GraphicsContext::bindTextureForRendering(FrameBufferObject *fbo) {
    if (fbo == currentFrameBuffer) return fbo;

    currentFrameBuffer = fbo;
    currentFrameBuffer->bind();
    return currentFrameBuffer;
}

GeometryKey GraphicsContext::bindGeometry(const GeometryKey &geometry) {
    if (geometry == currentGeometry) return geometry;

    GeometryQuery gq(*renderer);

    auto& geoRef = gq.getGeometry(geometry);
    glBindVertexArray(geoRef.gpuVAO);

    GeometryKey old = currentGeometry;
    currentGeometry = geometry;
    return old;
}

void GraphicsContext::bindShaderProgram(const ShaderProgram *shader) {
    if (currentShaderProgram == shader) return;

    currentShaderProgram = shader;

    if (shader->id() == 0) {
        std::cout << "[RENDERER ERROR] Invalid Shader: " << std::endl;
        cexpr::require(false);
    }
    glUseProgram(shader->id());
}

void GraphicsContext::setScissorState(ScissorState scissor) {
    OPENGL_STATE_APPLICATION.ApplicationTable(OPENGL_STATE_APPLICATION.current.scissorState, scissor);
    OPENGL_STATE_APPLICATION.current.scissorState = scissor;
}

void GraphicsContext::bindBuffer(BufferTarget target, const BufferKey key, int bufferBindingIndex, size_t offset, size_t size) {
    auto& buffer = key.buffer;

    buffer->clamp(offset, size);

    if (target != BufferTarget::SHADER_STORAGE_BUFFER) {
        assert(false);
    }
    assert(buffer->backingBuffer->gpuSSBO);

    if (size == 0) {
        std::cout << "[RENDERER ERROR] Invalid buffer size for Index: " << bufferBindingIndex << std::endl;
        assert(false);
    }
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bufferBindingIndex, buffer->backingBuffer->gpuSSBO, buffer->offset + offset, size);
}

void GraphicsContext::bindIndirectDrawBuffer(BufferKey key) {
    if (drawIndirectBuffer == key) return;
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, key.buffer->backingBuffer->gpuSSBO);
    drawIndirectBuffer = key;
}

void GraphicsContext::bindIndirectComputeBuffer(BufferKey key) {
    if (computeIndirectBuffer == key) return;
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, key.buffer->backingBuffer->gpuSSBO);
    computeIndirectBuffer = key;
}
