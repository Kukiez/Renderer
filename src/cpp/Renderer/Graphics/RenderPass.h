#pragma once
#include <Renderer/Renderer.h>
#include <Renderer/Graphics/Passes/IRenderPass.h>

#include "Passes/ClearPass.h"
#include <Renderer/Graphics/GraphicsPass.h>

class RenderPass {
    std::string_view allocateString(std::string_view name) const {
        auto mem = allocator->allocate<char>(name.length() + 1);
        std::memcpy(mem, name.data(), name.length());
        mem[name.length()] = '\0';

        return std::string_view(mem, name.length());
    }

    void emplacePass(IRenderPass* pass) {
        if (capPasses == 0) {
            capPasses = 2;
            graphicPasses = allocator->allocate<IRenderPass*>(capPasses);
        } else if (numPasses == capPasses) {
            auto newCommands = allocator->allocate<IRenderPass*>(capPasses * 2);
            std::memcpy(newCommands, graphicPasses, sizeof(IRenderPass*) * numPasses);

            graphicPasses = newCommands;

            capPasses *= 2;
        }
        graphicPasses[numPasses++] = pass;
    }

    void emplaceFeature(GraphicsFeature* feature) {
        if (capGraphicsFeatures == 0) {
            capGraphicsFeatures = 2;
            graphicFeatures = allocator->allocate<GraphicsFeature*>(capGraphicsFeatures);
        } else if (numGraphicsFeatures == capGraphicsFeatures) {
            auto newFeatures = allocator->allocate<GraphicsFeature*>(capGraphicsFeatures * 2);
            std::memcpy(newFeatures, graphicFeatures, sizeof(GraphicsFeature*) * numGraphicsFeatures);

            graphicFeatures = newFeatures;
            capGraphicsFeatures *= 2;
        }
        graphicFeatures[numGraphicsFeatures++] = feature;
    }

    Renderer* renderer{};
    GraphicsAllocator* allocator = nullptr;

    IRenderPass** graphicPasses = nullptr;
    size_t numPasses = 0;
    size_t capPasses = 0;

    GraphicsFeature** graphicFeatures = nullptr;
    size_t numGraphicsFeatures = 0;
    size_t capGraphicsFeatures = 0;
public:
    RenderPass() = default;

    RenderPass(Renderer* renderer, GraphicsAllocator* allocator = nullptr, const size_t passes = 1, const size_t features = 0)
    : renderer(renderer), allocator(allocator), capPasses(passes), capGraphicsFeatures(features)
    {
        if (!this->allocator) {
            this->allocator = renderer->getRenderAllocator();
        }
        if (features) {
            graphicFeatures = this->allocator->allocate<GraphicsFeature*>(features);
        }
        if (capPasses) {
            graphicPasses = this->allocator->allocate<IRenderPass*>(capPasses);
        }
    }

    template <typename Pass, typename... Args>
    requires std::is_base_of_v<IRenderPass, Pass> && (!std::is_same_v<Args, Pass> && ...)
    Pass& createPass(const std::string_view name, Args&&... args) {
        auto* pass = allocator->allocate<Pass>();

        if constexpr (std::is_base_of_v<IGraphicsPass, Pass>) {
            new (pass) Pass(allocateString(name), allocator, renderer->getShaderProgram(args...));
        } else {
            new (pass) Pass(allocateString(name), std::forward<Args>(args)...);
        }
        emplacePass(pass);
        return *pass;
    }

    BufferBindingsSet* createBindingsSet(size_t numBindings = 1) {
        return new (allocator->allocate<BufferBindingsSet>(numBindings)) BufferBindingsSet(allocator, numBindings);
    }

    template <typename Pass>
    std::decay_t<Pass>& createPass(Pass&& pass) {
        using TPass = std::decay_t<Pass>;
        auto* mem = allocator->allocate<TPass>();

        new (mem) TPass(std::forward<Pass>(pass));
        emplacePass(mem);
        return *mem;
    }

    GraphicsPass& createGraphicsPass(std::string_view name, const ShaderKey shader) {
        return createPass<GraphicsPass>(name, shader);
    }

    ClearPass& createClearPass(const RenderTexture& texture, const glm::vec4& color, ClearTarget targets) {
        return createPass<ClearPass>("ClearPass", texture, color, targets);
    }

    ClearPass& createClearPass(std::string_view name, const RenderTexture& texture, const glm::vec4& color) {
        return createPass<ClearPass>(name, texture, color);
    }

    template <typename Feature, typename... Args>
    Feature* createFeature(Args&&... args) {
        auto mem = allocator->allocate<Feature>();
        new (mem) Feature(std::forward<Args>(args)...);
        emplaceFeature(mem);
        return mem;
    }

    auto passes() const {
        return mem::make_range(graphicPasses, numPasses);
    }

    void reset() {
        graphicPasses = nullptr;
        numPasses = 0;
        capPasses = 0;

        graphicFeatures = nullptr;
        numGraphicsFeatures = 0;
        capGraphicsFeatures = 0;
    }
};
