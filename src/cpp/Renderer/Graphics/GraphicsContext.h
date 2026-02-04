#pragma once
#include <Renderer/Renderer.h>
#include <Renderer/Resource/Geometry/GeometryKey.h>
#include <Renderer/Resource/Buffer/BufferKey.h>
#include <Renderer/Graphics/State/RenderState.h>

struct MultiRenderTexture;
class FrameBufferObject;
struct RenderTexture;
struct GPUBuffer;
class Renderer;
struct Geometry;
class ShaderProgram;

enum class BufferTarget : uint8_t {
    SHADER_STORAGE_BUFFER,
    UNIFORM_BUFFER,
    INDIRECT_DRAW_BUFFER,
    INDIRECT_COMPUTE_BUFFER
};

class RENDERERAPI GraphicsContext {
    Renderer* renderer;
    RenderState currentState{};
    const ShaderProgram* currentShaderProgram = nullptr;
    FrameBufferObject* currentFrameBuffer{};
    GeometryKey currentGeometry{};

    BufferKey drawIndirectBuffer{};
    BufferKey computeIndirectBuffer{};
public:
    explicit GraphicsContext(Renderer* renderer) : renderer(renderer) {}

    const Geometry* getGeometry(const GeometryKey& key) const;

    const RenderState& getCurrentState() const { return currentState; }

    void setCurrentState(const RenderState& state);

    FrameBufferObject* bindTextureForRendering(const RenderTexture& texture);
    FrameBufferObject* bindTextureForRendering(const MultiRenderTexture& texture);
    FrameBufferObject* bindTextureForRendering(FrameBufferObject* fbo);

    GeometryKey bindGeometry(const GeometryKey& geometry);

    void bindShaderProgram(const ShaderProgram* shader);

    const ShaderProgram* getCurrentShaderProgram() const { return currentShaderProgram; }

    Renderer* getRenderer() const { return renderer; }

    void setScissorState(ScissorState scissor);

    void bindBuffer(BufferTarget target, BufferKey key, int bufferBindingIndex, size_t offset = 0, size_t size = -1);

    void bindIndirectDrawBuffer(BufferKey key);
    void bindIndirectComputeBuffer(BufferKey key);
};