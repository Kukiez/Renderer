#pragma once
#include <Renderer/RenderingStages/RenderingPass.h>
#include <UI/Rendering/UIStyleType.h>

class UIPassInvocation;

struct UIRenderingPass : IRenderingPass<UIRenderingPass> {};

template <typename P>
concept HasOnRenderUI = requires(P& pipeline, UIPassInvocation& pass)
{
    pipeline.onRender(pass);
};

template <ui::IsUIStyle Style>
struct UIPrimitivePipeline {
    using StyleType = Style;

    constexpr void IsValid(this auto self) {
        static_assert(HasOnRenderUI<decltype(self)>, "D");
    }
};

template <typename P>
concept IsUIPipeline = cexpr::is_base_of_template<UIPrimitivePipeline, P>;

struct VPrimitivePipeline {
    using OnRenderFn = void(*)(const void* inst, const UIPassInvocation& pass);
    mem::typeindex type{};
    OnRenderFn onRender{};

    template <IsUIPipeline T>
    static constexpr VPrimitivePipeline create() {
        VPrimitivePipeline p;
        p.type = mem::type_info::of<T>();
        p.onRender = [](const void* inst, const UIPassInvocation& pass) {
            if constexpr (std::is_empty_v<T>) {
                constexpr static T t;
                t.onRender(pass);
            } else {
                static_cast<const T*>(inst)->onRender(pass);
            }
        };
        return p;
    }
};