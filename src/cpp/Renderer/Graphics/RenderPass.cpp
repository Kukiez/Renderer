#include "RenderPass.h"

#include <openGL/Shader/ShaderCompiler.h>

#include "GraphicsContext.h"

IGraphicsPass::IGraphicsPass(std::string_view name, GraphicsAllocator* allocator, const ShaderProgram* shader):
    IRenderPass(name),
    allocator(allocator),
    bindings(allocator, 0),
    shader(shader),
    pushConstantsBlock(PushConstantSet::allocate(allocator, shader))
{}

void IGraphicsPass::bind(BufferBindingsSet &set, const ShaderProgram *program, BufferKey key, std::string_view name) {
    if (key == NULL_BUFFER_KEY) {
        std::cout << "Invalid key" << " for: " << name << std::endl;
        assert(false);
    }

    const auto [bufferIndex, localIndex] = program->definition().getBufferBinding(name);

    if (bufferIndex == BufferBindingIndex::INVALID) {
        return;
    }

    set.add({key, static_cast<int>(bufferIndex)});
}

void IGraphicsPass::bind(BufferKey key, const std::string_view name) {
    bind(bindings, shader, key, name);
}

void IGraphicsPass::bindSet(const BufferBindingsSet *set) {
    auto sets = allocator->allocate<const BufferBindingsSet*>(numBindingSets + 1);
    if (numBindingSets != 0) {
        std::memcpy(sets, bindingsSets, sizeof(BufferBindingsSet*) * numBindingSets);
    }
    sets[numBindingSets] = set;
    bindingsSets = sets;
    ++numBindingSets;
}

void IGraphicsPass::bindPass(GraphicsContext &ctx) const {
    ctx.bindShaderProgram(shader);

    pushConstantsBlock.bind(ctx);

    for (auto& set : mem::make_range(bindingsSets, numBindingSets)) {
        set->bind(ctx);
    }
    bindings.bind(ctx);
}
