#pragma once
#include "StandardDrawContainer.h"
#include "../GraphicsPass.h"
#include "../DrawPrimitive.h"

struct ScissorDrawCommand : DrawCommand {
    ScissorState scissor{};
};

class RENDERERAPI ScissorDrawContainer final : public GraphicsDrawContainer {
    GraphicsAllocator* allocator{};
    ScissorDrawCommand* commands{};
    size_t numCommands = 0;
    size_t capCommands = 0;
public:
    explicit ScissorDrawContainer(GraphicsAllocator* allocator, size_t capCommands = 8);

    void draw(const ScissorDrawCommand& command);

    void draw(GraphicsContext& ctx) override;

    static void draw(GraphicsContext& ctx, const ScissorDrawCommand* commands, const size_t numCommands);
};