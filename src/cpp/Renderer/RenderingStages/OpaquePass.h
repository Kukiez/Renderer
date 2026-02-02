#pragma once
#include <Renderer/Pipeline/GraphicsPassBase.h>
#include <Renderer/Pipeline/PassInvocation.h>
#include <Renderer/Skybox/SkyboxSystem.h>

#include "RenderingPass.h"

struct OpaqueRenderingPass : IRenderingPass<OpaqueRenderingPass> {};

struct OpaquePass : GraphicsPassBase<OpaquePass>, IPassInvocation<OpaquePass> {
    Skybox skybox;

    using GraphicsPassBase::onExecute;

    OpaquePass(std::string_view name,
        const RenderingPipeline* pipeline,
        const VisiblePrimitiveList* visible,
        const MultiRenderTexture &renderTarget,
        const GraphicsPassView &view,
        Skybox skybox
    ) : GraphicsPassBase(name, pipeline, visible, renderTarget, {}, view, RenderingPassType::of<OpaqueRenderingPass>()), skybox(skybox) {
        RenderState opaqueState{};
        opaqueState.depthState.func = DepthFunction::Less;
        opaqueState.depthState.writeEnabled = true;
        opaqueState.depthState.enabled = true;
        opaqueState.cullMode = CullMode::Back;

        state = opaqueState;
    }

    OpaquePass(const Skybox& skybox) : skybox(skybox) {}

    void onPassBegin(const GraphicsPassInvocationBase& invocation);

    void onPassEnd(const GraphicsPassInvocationBase& invocation);
};

struct DepthRenderingPass : IRenderingPass<DepthRenderingPass> {};

struct DepthPass : GraphicsPassBase<DepthPass>, IPassInvocation<DepthPass> {
    using GraphicsPassBase::onExecute;

    DepthPass(
        std::string_view name,
        const RenderingPipeline* pipeline,
        const VisiblePrimitiveList* visible,
        const MultiRenderTexture &renderTarget,
        const RenderState &state,
        const GraphicsPassView &view
    ) : GraphicsPassBase(name, pipeline, visible, renderTarget, state, view, RenderingPassType::of<DepthRenderingPass>()) {}

    void onPassBegin(const GraphicsPassInvocationBase& invocation) {}
    void onPassEnd(const GraphicsPassInvocationBase& invocation) {}
};

struct TransparentRenderingPass : IRenderingPass<TransparentRenderingPass> {
    void onPassBegin(const GraphicsPassInvocationBase& invocation);
    void onPassEnd(const GraphicsPassInvocationBase& invocation);
};


