#pragma once
#include "../GraphicsPass.h"
#include "../DrawPrimitive.h"

struct DrawRange {
    union {
        unsigned indexCount = 0;
        unsigned vertexCount;
    };

    unsigned firstIndex = 0;
    unsigned baseVertex = 0;
    unsigned instanceCount = 1;
    unsigned firstInstance = 0;

    DrawRange() = default;
    DrawRange(const unsigned baseVertex, const unsigned firstIndex, const unsigned indexCount, const unsigned firstInstance = 0, const unsigned instanceCount = 1)
        : indexCount(indexCount), firstIndex(firstIndex), baseVertex(baseVertex), instanceCount(instanceCount), firstInstance(firstInstance) {}

    size_t hash() const {
        return indexCount ^ firstIndex ^ baseVertex ^ instanceCount ^ firstInstance;
    }

    const void* indexOffset() const {
        return reinterpret_cast<const void*>(firstIndex * sizeof(unsigned));
    }

    bool operator == (const DrawRange& other) const {
        return indexCount == other.indexCount && firstIndex == other.firstIndex && baseVertex == other.baseVertex && instanceCount == other.instanceCount && firstInstance == other.firstInstance;
    }

    bool operator != (const DrawRange& other) const {
        return !(*this == other);
    }
};

struct DrawCommand {
    GeometryKey geometry{};
    DrawRange drawRange{};
    DrawPrimitive drawPrimitive = DrawPrimitive::TRIANGLES;
};

class RENDERERAPI StandardDrawContainer final : public GraphicsDrawContainer {
    GraphicsAllocator* allocator{};
    DrawCommand* commands{};
    size_t numCommands = 0;
    size_t capCommands = 0;
public:
    explicit StandardDrawContainer(GraphicsAllocator* allocator, size_t capCommands = 8);

    void draw(const DrawCommand& command);

    void draw(GraphicsContext& ctx) override;

    static void draw(GraphicsContext& ctx, const DrawCommand* commands, size_t numCommands);
};