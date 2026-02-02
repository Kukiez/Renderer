#pragma once
#include <Renderer/Frame.h>
#include <Renderer/Graphics/RenderPass.h>
#include <Renderer/Scene/Primitives/PrimitivePipeline.h>
#include <Renderer/Scene/Primitives/VisiblePrimitive.h>
#include <Renderer/Common/GPUCamera.h>
#include <Renderer/Resource/Buffer/BufferComponentType.h>

template <typename T>
struct GraphicsPassBase {
    std::string_view name;
    const RenderingPipeline* pipeline{};
    const VisiblePrimitiveList* visible{};
    MultiRenderTexture renderTarget{};
    RenderState state{};
    GraphicsPassView view{};
    RenderingPassType passType{};

    void onExecute(const Frame* frame, const PassInvocationID id) {
        auto [viewBuffer, vDat] = frame->getRenderer()->getBufferStorage().createBufferWithData<GPUCamera>(1, BufferUsageHint::FRAME_SCRATCH_BUFFER);

        auto& gpuCamera = vDat[0];
        gpuCamera.cameraPosition = view.position;
        gpuCamera.width = view.width;
        gpuCamera.height = view.height;
        gpuCamera.nearClip = view.nearPlane;
        gpuCamera.farClip = view.farPlane;
        gpuCamera.projection = view.projection;
        gpuCamera.view = view.view;
        gpuCamera.ortho = glm::ortho(0.0f, view.width, view.height, 0.f);
        gpuCamera.inverseProjection = glm::inverse(view.projection);
        gpuCamera.inverseView = glm::inverse(view.view);

        auto renderer = frame->getRenderer();

        auto* passes = frame->getRenderer()->getRenderAllocator()->allocate<RenderPass>(pipeline->getEffectiveSize());

        for (size_t i = 0; i < pipeline->getEffectiveSize(); ++i) {
            new (passes + i) RenderPass(renderer);
        }

        GraphicsPassInvocationBase invocation(
            name,
            renderer,
            frame,
            view,
            viewBuffer,
            renderTarget,
            state,
            passType,
            visible,
            passes
        );

        size_t instID = 0;
        for (auto& [inst, vt] : pipeline->getPipelines()) {
            if (!inst) {
                continue;
            }
            GraphicsPassInvocation instance(&invocation, instID);
            ++instID;
            vt.onRender(inst, instance);
        }

        GraphicsContext ctx(renderer);

        ctx.bindTextureForRendering(renderTarget);
        ctx.setCurrentState(state);

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, name.length(), name.data());

        renderer->synchronize();
        static_cast<T *>(this)->onPassBegin(invocation);
        for (size_t i = 0; i < instID; ++i) {
            for (auto& pass : passes[i].passes()) {
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, pass->name().length(), pass->name().data());
                pass->render(ctx);
                glPopDebugGroup();
            }
        }
        static_cast<T *>(this)->onPassEnd(invocation);
        glPopDebugGroup();
    }
};
