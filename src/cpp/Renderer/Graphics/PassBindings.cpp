#include "PassBindings.h"

#include <openGL/Shader/ShaderCompiler.h>
#include <openGL/Shader/ShaderReflection.h>

#include "Features.h"
#include "GraphicsContext.h"

BufferBindingsSet::BufferBindingsSet(GraphicsAllocator *allocator, size_t capBindings) : allocator(allocator), capBindings(capBindings) {
    if (capBindings != 0) {
        bindings = allocator->allocate<BufferBinding>(capBindings);
    }
}

void BufferBindingsSet::add(const BufferBinding &binding) {
    if (!capBindings) {
        bindings = allocator->allocate<BufferBinding>(4);
        capBindings = 4;
    } else if (numBindings == capBindings) {
        auto newFeatures = allocator->allocate<BufferBinding>(capBindings * 2);

        for (size_t i = 0; i < numBindings; ++i) {
            newFeatures[i] = bindings[i];
        }
        bindings = newFeatures;
        capBindings *= 2;
    }
    bindings[numBindings++] = binding;
}