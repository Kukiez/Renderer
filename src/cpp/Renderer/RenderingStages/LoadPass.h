#pragma once
#include <ECS/System/SystemQuery.h>
#include <ECS/System/Stages/StageInterface.h>

struct RendererLoadResources;
class RenderPass;
class Renderer;

class RendererLoadView {
    RendererLoadResources* self;
    Renderer* renderer{};
public:
    RendererLoadView(RendererLoadResources* self, Renderer* renderer) : self(self), renderer(renderer) {}

    operator Level& () { return reinterpret_cast<Level &>(*renderer); }

    template <typename T>
    T& getResource() {
        const SystemQuery sq(reinterpret_cast<Level &>(*renderer));
        return *sq.getSystem<T>();
    }

    Renderer& getRenderer() { return *renderer; }
};

struct RendererLoadResources : Stage<RendererLoadResources> {
    struct CallingConvention {
        template <typename Stage, typename System>
        static void invoke(Stage& stage, System& system, LevelContext& level, SystemCallContext& ctx) {
            RendererLoadView view(&stage, stage.renderer);
            system.onLoad(view);
        }
    };

    using stage = RendererLoadResources;

    template <typename T>
    constexpr static bool HasFunction = requires
    {
        &T::onLoad;
    };

    static constexpr auto ExecutionModel = StageExecutionModel::SERIAL;
    static constexpr auto ScheduleModel = StageScheduleModel::MANUAL;;

    Renderer* renderer{};

    explicit RendererLoadResources(Renderer* renderer) : renderer(renderer) {}
};