#pragma once
#include <openGL/BufferObjects/FrameBufferObject.h>
#include <Renderer/Graphics/State/GLRenderState.h>
#include "../Features.h"

struct FramebufferRenderTargetBlend {
    FramebufferAttachmentIndex attachment{};
    BlendFunction srcFunction{};
    BlendFunction dstFunction{};
    BlendEquation equation{};
};

class FramebufferIndexedBlend final : public GraphicsFeature {
    std::vector<FramebufferRenderTargetBlend> blendTargets;
public:
    void addBlend(FramebufferAttachmentIndex attachment, BlendFunction srcFunction, BlendFunction dstFunction, BlendEquation equation){
        blendTargets.emplace_back(FramebufferRenderTargetBlend{attachment, srcFunction, dstFunction, equation});
    }

    void push(GraphicsContext &renderer) override {
        for (const auto [attachment, srcFn, dstFn, eq] : blendTargets) {
            const int attachmentIndex = static_cast<int>(attachment - FramebufferAttachmentIndex::COLOR_0);
            glEnablei(GL_BLEND, attachmentIndex);
            glBlendFunci(attachmentIndex, opengl_enum_cast(srcFn), opengl_enum_cast(dstFn));
            glBlendEquationi(attachmentIndex, opengl_enum_cast(eq));
        }
    }

    void pop(GraphicsContext &renderer) override {
        for (const auto [attachment, srcFn, dstFn, eq] : blendTargets) {
            const int attachmentIndex = static_cast<int>(attachment - FramebufferAttachmentIndex::COLOR_0);
            glDisablei(GL_BLEND, attachmentIndex);
        }
    }

    std::string_view name() const override { return "FramebufferIndexedBlend"; }
};