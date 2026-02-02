#pragma once
#include "Pipeline/PassInvocation.h"

class Renderer;

class Frame {
    struct InvocationData {
        void* instance{};
        PassInvocation vtable{};
        std::string_view name{};
    };
    Renderer* renderer{};
    std::vector<std::pair<void*, PassInvocation>> invocations{};
public:
    explicit Frame(Renderer* renderer) : renderer(renderer) {}

    template <IsPassInvocation P>
    PassInvocationID createInvocationPass(P&& desc) {
        using T = std::decay_t<P>;

        void* invData = renderer->getRenderAllocator()->allocateAsTrivial<T>(1);

        new (invData) T(std::forward<P>(desc));
        auto vt = PassInvocation::create<T>();

        invocations.emplace_back(invData, vt);

        return PassInvocationID{static_cast<unsigned>(invocations.size()) - 1};
    }

    void render() {
        unsigned id = 0;
        for (auto& [invData, invLogic] : invocations) {
            invLogic.onExecute(invData, this, PassInvocationID{id});
            ++id;
        }
    }

    Renderer* getRenderer() const { return renderer; }

    GraphicsAllocator* getFrameAllocator() const {
        return renderer->getRenderAllocator();
    }

    VisiblePrimitiveList* createVisibleList(PrimitiveWorld* world) const {
        auto mem = renderer->getRenderAllocator()->allocateAsTrivial<VisiblePrimitiveList>(1);

        new (mem) VisiblePrimitiveList(renderer, world);
        return mem;
    }
};