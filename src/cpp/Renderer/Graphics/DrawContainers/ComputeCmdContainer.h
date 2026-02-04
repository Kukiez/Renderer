#pragma once
#include "RendererAPI.h"

struct ComputeCommand {
    glm::ivec3 invocations{};
};

struct IndirectComputeCommand {
    BufferKey indirect{};
    size_t offset = 0;
};

class ComputeCmdContainer final : public GraphicsDrawContainer {
    GraphicsAllocator* allocator{};
    ComputeCommand* commands{};
    size_t numCommands = 0;
    size_t capCommands = 0;
public:
    RENDERERAPI explicit ComputeCmdContainer(GraphicsAllocator* allocator, size_t capCommands = 2);

    RENDERERAPI void dispatch(const ComputeCommand& command);
    RENDERERAPI void dispatch(int x, int y, int z);

    void draw(GraphicsContext& ctx) override;
};

class IndirectComputeCmdContainer final : public GraphicsDrawContainer {
    GraphicsAllocator* allocator{};
    IndirectComputeCommand* commands{};
    uint32_t numCommands = 0;
    uint32_t capCommands = 0;
public:
    RENDERERAPI explicit IndirectComputeCmdContainer(GraphicsAllocator* allocator, size_t capCommands = 1);

    RENDERERAPI void dispatch(const IndirectComputeCommand& command);

    void draw(GraphicsContext& ctx) override;
};
