#pragma once
#include <UI/Rendering/UIRenderingContext.h>
#include <Renderer/Pipeline/PassInvocation.h>

#include "UIRenderingPass.h"

class UIRenderingPipeline {
    std::vector<std::pair<VPrimitivePipeline, const void*>> pipelines;
public:
    template <IsUIPipeline P>
    void addPipeline(const P* pipeline) {
        using StyleType = P::StyleType;

        auto id = ui::UIStyleTypes.of<StyleType>();

        if (pipelines.size() <= id.id()) pipelines.resize(id.id() + 1);

        pipelines[id.id()] = {
            VPrimitivePipeline::create<P>(), pipeline
        };
    }

    bool run(const ui::UIStyleType::TypeID style, const UIPassInvocation& inv) const {
        auto [pipeline, data] = pipelines[style.id()];

        if (pipeline.onRender) {
            pipeline.onRender(data, inv);
            return true;
        }
        return false;
    }
};

class UIPass : IPassInvocation<UIPass> {
    ui::UIRenderingContext* context{};
    const UIRenderingPipeline* pipeline;
    RenderTexture renderTarget;
public:
    UIPass(ui::UIRenderingContext* drawCommands, const UIRenderingPipeline* pipeline, const RenderTexture &renderTarget) : context(drawCommands), pipeline(pipeline), renderTarget(renderTarget) {}

    void onExecute(const Frame* frame, PassInvocationID id);
};