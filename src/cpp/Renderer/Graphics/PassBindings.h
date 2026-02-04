#pragma once
#include "GraphicsContext.h"

struct GraphicsAllocator;
class ShaderProgram;
class GraphicsContext;

struct BufferBinding {
    BufferKey buffer{};
    int index{};
    size_t offset = 0;
    size_t size = std::numeric_limits<size_t>::max();

    bool isRange() const {
        return size != std::numeric_limits<size_t>::max() || offset != 0;
    }
};

class RENDERERAPI BufferBindingsSet {
    GraphicsAllocator* allocator{};
    BufferBinding* bindings{};
    size_t numBindings{};
    size_t capBindings{};
public:
    BufferBindingsSet(GraphicsAllocator* allocator, size_t capBindings);

    void add(const BufferBinding& binding);

    void bind(GraphicsContext& ctx) const {
        for (auto& binding : mem::make_range(bindings, numBindings)) {
            ctx.bindBuffer(BufferTarget::SHADER_STORAGE_BUFFER, binding.buffer, binding.index, binding.offset, binding.size);
        }
    }
};