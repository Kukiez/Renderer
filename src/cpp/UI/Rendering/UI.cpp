#include <Renderer/Frame.h>
#include <Renderer/Pipeline/GraphicsPassBase.h>

#include "UIPass.h"
#include "UIPassInvocation.h"

void UIPass::onExecute(const Frame *frame, PassInvocationID id) {
    auto renderer = frame->getRenderer();

    auto& drawCommands = context->getDrawCommands();

    if (drawCommands.empty()) return;

    size_t first = 0;
    size_t last = 0;
    auto style = drawCommands[0].getStyleType();

    struct UIDrawRange {
        ui::UIStyleType::TypeID rendererID{};
        size_t first{};
        size_t last{};
    };

    std::vector<UIDrawRange> ranges;
    ranges.reserve(50);

    auto [viewBuffer, viewData] = renderer->getBufferStorage().createBufferWithData<GPUCamera>(1, BufferUsageHint::FRAME_SCRATCH_BUFFER);

    viewData[0].ortho = glm::ortho(0.f, 2560.f, 1440.f, 0.f);

    bool isDone = false;
    while (!isDone) {
        if (last == drawCommands.size()) {
            isDone = true;
        } else {
            auto& currCmd = drawCommands[last];

            if (style == currCmd.getStyleType()) {
                ++last;
                continue;
            }
        }
        ranges.emplace_back(style,  first, last);

        if (!isDone) {
            auto& currCmd = drawCommands[last];
            first = last;
            style = currCmd.getStyleType();
            ++last;
        }
    }

    RenderPass mainPass(renderer, renderer->getRenderAllocator(), ranges.size());

    for (auto& range : ranges) {
        UIPassInvocation invocation(
            frame, viewBuffer, drawCommands.data(), range.first, range.last - range.first, &mainPass
        );
        bool r = pipeline->run(range.rendererID, invocation);

        assert(r);
    }

    RenderState state;
    state.blendState.enabled = true;
    state.cullMode = CullMode::None;

    GraphicsContext ctx(renderer);
    ctx.bindTextureForRendering(renderTarget);
    ctx.setCurrentState(state);

    renderer->synchronize();
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "UI Render");
    for (auto& pass : mainPass.passes()) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, pass->name().length(), pass->name().data());
        pass->render(ctx);
        glPopDebugGroup();
    }
    glPopDebugGroup();
}
