#pragma once
#include <Renderer/Resource/Buffer/BufferKey.h>
#include <Renderer/Resource/Texture/RenderTexture.h>
#include <Renderer/Scene/Primitives/VisiblePrimitive.h>
#include <Renderer/Graphics/RenderPass.h>

struct GraphicsPassView {
    glm::mat4 view{};
    glm::mat4 projection{};
    glm::mat4 ortho{};
    glm::mat4 inverseView{};
    glm::mat4 inverseProjection{};
    glm::vec3 position{};
    float nearPlane{};
    float farPlane{};
    float width{};
    float height{};
};

class GraphicsPassInvocationBase {
protected:
    friend class GraphicsPassInvocation;

    std::string_view name;

    GraphicsPassView view{};
    BufferKey viewBuffer{};

    MultiRenderTexture renderTarget{};
    RenderState state{};

    RenderingPassType passType{};

    Renderer* renderer{};
    const Frame* frame{};

    const VisiblePrimitiveList* visible{};

    RenderPass* passes{};
public:
    GraphicsPassInvocationBase(std::string_view name, Renderer* renderer, const Frame* frame, const GraphicsPassView &view, const BufferKey viewBuffer,
        const MultiRenderTexture &renderTarget, const RenderState &state, RenderingPassType passType, const VisiblePrimitiveList* visible, RenderPass* passes)
        : name(name), view(view), viewBuffer(viewBuffer), renderTarget(renderTarget), state(state),
    passType(passType), renderer(renderer), frame(frame), visible(visible), passes(passes)
    {}

    Renderer& getRenderer() const {
        return *renderer;
    }

    BufferKey getViewBuffer() const {
        return viewBuffer;
    }

    const GraphicsPassView& getView() const {
        return view;
    }

    const MultiRenderTexture& getRenderTarget() const {
        return renderTarget;
    }

    const RenderState& getState() const {
        return state;
    }

    RenderingPassType getPassType() const {
        return passType;
    }

    const VisiblePrimitiveList& getVisiblePrimitives() const {
        return *visible;
    }

    std::string_view getName() const {
        return name;
    }

    const Frame& getFrame() const {
        return *frame;
    }
};

enum class GraphicsInvocationInstanceID : size_t {};

class GraphicsPassInvocation {
    GraphicsPassInvocationBase* base{};
    size_t instanceID{};
public:
    GraphicsPassInvocation(GraphicsPassInvocationBase* base, const size_t instanceID) : base(base), instanceID(instanceID) {}

    Renderer& getRenderer() const {
        return base->getRenderer();
    }

    BufferKey getViewBuffer() const {
        return base->getViewBuffer();
    }

    const GraphicsPassView& getView() const {
        return base->getView();
    }

    const MultiRenderTexture& getRenderTarget() const {
        return base->getRenderTarget();
    }

    const RenderState& getState() const {
        return base->getState();
    }

    RenderingPassType getPassType() const {
        return base->getPassType();
    }

    const VisiblePrimitiveList& getVisiblePrimitives() const {
        return base->getVisiblePrimitives();
    }

    std::string_view getName() const {
        return base->getName();
    }

    const Frame& getFrame() const {
        return base->getFrame();
    }

    RenderPass& createRenderPass() const {
        return base->passes[instanceID];
    }

    GraphicsInvocationInstanceID getInstanceID() const {
        return static_cast<GraphicsInvocationInstanceID>(instanceID);
    }

    GraphicsAllocator* getPassAllocator() const {
        return base->getRenderer().getRenderAllocator();
    }
};