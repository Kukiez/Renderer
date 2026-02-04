#pragma once
#include <string_view>

#include "Features.h"
#include "PassBindings.h"
#include "Passes/IRenderPass.h"

struct RENDERERAPI GraphicsDrawContainer {
    virtual void draw(GraphicsContext& ctx) = 0;
};

template <typename GDC>
concept IsGraphicsDrawContainer = std::is_base_of_v<GraphicsDrawContainer, GDC>;

class RENDERERAPI GraphicsPass : public IGraphicsPass {
    GraphicsDrawContainer* drawContainer{};

    GraphicsFeature** features = nullptr;
    size_t numFeatures = 0;
    size_t capFeatures = 0;
public:
    using IGraphicsPass::IGraphicsPass;

    template <typename DrawContainer, typename... Args>
    DrawContainer& usingDrawContainer(Args&&... args) {
        drawContainer = allocator->allocate<DrawContainer>();
        return *new (drawContainer) DrawContainer(allocator, std::forward<Args>(args)...);
    }

    template <typename Push>
    auto push(std::string_view name, Push&& push) {
        return pushConstantsBlock.push(name, std::forward<Push>(push));
    }

    GraphicsFeature* usingFeature(GraphicsFeature* feature);

    const GraphicsAllocator* getAllocator() const { return allocator; }

    GraphicsDrawContainer* getDrawContainer() const { return drawContainer; }

    void render(GraphicsContext& ctx) const override;
};