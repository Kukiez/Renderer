#pragma once
#include "Primitive.h"
#include <vector>

class GraphicsPassInvocation;

class RenderingPipeline {
    struct PipelineVTable {
        void (*onRender)(const void* pipeline, GraphicsPassInvocation& invocation){};
        mem::typeindex type{};

        template <typename T>
        static PipelineVTable of() {
            return { [](const void* p, GraphicsPassInvocation& inv) {
                static_cast<const T*>(p)->onRender(inv);
            }, mem::type_info::of<T>()
            };
        }
    };

    void addPipeline(const PrimitiveCollectionType id, const void* pipeline, PipelineVTable vtable) {
        if (pipelines.size() <= id.id()) {
            pipelines.resize(id.id() + 1);
        }
        if (pipelines[id.id()].first == nullptr) {
            ++effectiveSize;
        }
        pipelines[id.id()] = { pipeline, vtable};
    }

    std::vector<std::pair<const void*, PipelineVTable>> pipelines{};
    size_t effectiveSize = 0;
public:
    RenderingPipeline() = default;

    template <IsStaticPrimitivePipeline P>
    void addPipeline(const P* pipeline) {
        addPipeline(
            PrimitiveCollectionType::of<typename P::CollectionType>(),
            pipeline,
            PipelineVTable::of<P>()
        );
    }

    template <IsPrimitiveCollection C, IsDynamicPrimitivePipeline P>
    void addPipeline(const P* pipeline) {
        addPipeline(PrimitiveCollectionType::of<C>(), pipeline, PipelineVTable::of<P>());
    }

    auto& getPipelines() const {
        return pipelines;
    }

    size_t size() const {
        return pipelines.size();
    }

    size_t getEffectiveSize() const {
        return effectiveSize;
    }
};