#pragma once
#include <Renderer/Graphics/PushConstant.h>

#include "Renderer/Graphics/PassBindings.h"

class ShaderProgram;

class RENDERERAPI IRenderPass {
    std::string_view myName{};
public:
    IRenderPass(const std::string_view name) : myName(name) {}

    virtual void render(GraphicsContext& ctx) const = 0;

    std::string_view name() const { return myName; }
};

class RENDERERAPI IGraphicsPass : public IRenderPass {
protected:
    GraphicsAllocator* allocator{};

    const BufferBindingsSet* const* bindingsSets{};
    size_t numBindingSets{};

    BufferBindingsSet bindings;

    const ShaderProgram* shader{};

    PushConstantSet pushConstantsBlock;
public:
    IGraphicsPass(std::string_view name, GraphicsAllocator* allocator, const ShaderProgram* shader);

    static void bind(BufferBindingsSet& set, const ShaderProgram* program, BufferKey key, std::string_view name);

    void bind(BufferKey key, const std::string_view name);

    void bindSet(const BufferBindingsSet* set);

    void render(GraphicsContext &ctx) const override = 0;
protected:

    void bindPass(GraphicsContext& ctx) const;
};