#pragma once
#include "../GraphicsPass.h"

struct DrawIndirectKey {
    BufferKey buffer{};
    unsigned offset = 0;
    unsigned count = 1;
    unsigned stride = 0;
};

struct IndirectDrawCommand {
    GeometryKey geometry{};
    DrawIndirectKey indirect;
    DrawPrimitive drawPrimitive = DrawPrimitive::TRIANGLES;
};

class RENDERERAPI IndirectDrawContainer : public GraphicsDrawContainer {
    GraphicsAllocator* allocator{};
    IndirectDrawCommand* commands{};
    size_t numCommands = 0;
    size_t capCommands = 0;
public:
    explicit IndirectDrawContainer(GraphicsAllocator* allocator, size_t capCommands = 8);

    void draw(const IndirectDrawCommand& command);

    void draw(GraphicsContext& ctx) override;
};

struct DrawElementsIndirectCommand {
    unsigned indexCount = 0;
    unsigned instanceCount = 1;
    unsigned firstIndex = 0;
    unsigned baseVertex = 0;
    unsigned baseInstance = 0;
};

class RENDERERAPI StandardElementsIndirectMultiDrawContainer : public GraphicsDrawContainer {
    struct IndirectRange {
        GeometryKey geometry{};
        DrawPrimitive primitive{};
        unsigned offset = 0;
        unsigned count = 0;
    };
    GraphicsAllocator* allocator{};

    BufferKey indirectBuffer;
    TMappedBufferRange<DrawElementsIndirectCommand> mappedBuffer;

    size_t nextIndirectIdx = 0;

    IndirectRange* ranges{};
    size_t numRanges = 0;
    size_t capRanges = 0;
public:
    StandardElementsIndirectMultiDrawContainer(GraphicsAllocator* allocator, BufferKey indirectBuffer, const size_t maxDrawCommands, size_t numStartingIndirectRanges = 4)
        : allocator(allocator), indirectBuffer(indirectBuffer), mappedBuffer(indirectBuffer.mapRange<DrawElementsIndirectCommand>(0, maxDrawCommands)), capRanges(numStartingIndirectRanges) {
        if (numStartingIndirectRanges == 0) {
            capRanges = 1;
        }
        ranges = allocator->allocate<IndirectRange>(capRanges);
    }

    void draw(const DrawCommand& cmd);

    void draw(GraphicsContext& ctx) override;
};

