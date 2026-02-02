#pragma once

class UIPassInvocation {
    BufferKey camera{};

    const Frame* frame{};
    const ui::UIDrawCommand* drawCommands{};
    size_t offset{};
    size_t numDrawCommands{};

    RenderPass* pass{};
public:
    UIPassInvocation(const Frame* frame, BufferKey camera, const ui::UIDrawCommand* drawCommands, size_t offset, size_t numDrawCommands, RenderPass* pass)
    : frame(frame), drawCommands(drawCommands), offset(offset), numDrawCommands(numDrawCommands), pass(pass), camera(camera) {}

    const ui::UIDrawCommand* getDrawCommands() const { return drawCommands + offset; }

    size_t getNumDrawCommands() const { return numDrawCommands; }

    const Frame* getFrame() const { return frame; }

    Renderer& getRenderer() const {
        return *frame->getRenderer();
    }

    RenderPass& createRenderPass() const {
        return *pass;
    }

    BufferKey getCameraBuffer() const { return camera; }
};