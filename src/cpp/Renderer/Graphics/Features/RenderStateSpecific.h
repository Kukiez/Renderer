#pragma once
#include "../Features.h"
#include <Renderer/Graphics/State/RenderState.h>

class RenderStateSpecific final : public GraphicsFeature {
    RenderState oldState{};
    RenderState state{};
public:
    RenderStateSpecific() = default;
    RenderStateSpecific(const RenderState &state) : state(state) {}

    void setRenderState(const RenderState &state) { this->state = state; }

    const RenderState& getState() const { return state; }

    void push(GraphicsContext &renderer) override;

    std::string_view name() const override { return "RenderStateSpecific"; }
};