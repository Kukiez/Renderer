#pragma once
#include <glm/vec4.hpp>
#include <openGL/BufferObjects/FrameBufferObject.h>

#include "../Features.h"

struct FramebufferClearColor {
    FramebufferAttachmentIndex attachment{};
    glm::vec4 color{};
};

class FramebufferIndexedClear final : public GraphicsFeature {
    std::vector<FramebufferClearColor> clearColors;
public:
    void addClearColor(const FramebufferAttachmentIndex attachment, const glm::vec4& color) {
        clearColors.emplace_back(FramebufferClearColor{attachment, color});
    }

    void push(GraphicsContext &renderer) override {
        for (const auto [attachment, color] : clearColors) {
            if (attachment == FramebufferAttachmentIndex::DEPTH) {
                glClearBufferfv(GL_DEPTH, 0, &color[0]);
            } else if (attachment >= FramebufferAttachmentIndex::COLOR_0
                && attachment <= FramebufferAttachmentIndex::COLOR_7)
            {
                const int index = static_cast<int>(attachment) - static_cast<int>(FramebufferAttachmentIndex::COLOR_0);
                glClearBufferfv(GL_COLOR, index, &color[0]);
            }
        }
    }

    std::string_view name() const override { return "FramebufferIndexedClear"; }

};