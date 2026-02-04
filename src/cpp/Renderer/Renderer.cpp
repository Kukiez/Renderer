#include "Renderer.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/RenderPass.h"
#include <Renderer/Resource/Texture/TextureResourceType.h>
#include <Renderer/Resource/Geometry/GeometryComponentType.h>

#include "RenderingStages/LoadPass.h"
#include "Resource/Buffer/BufferComponentType.h"
#include "Resource/Geometry/GeometryBuilder.h"
#include "Resource/Material/MaterialStorage.h"

struct AllocationRecord {
    mem::type_info type;
    size_t count;
};

static tbb::enumerable_thread_specific<std::vector<AllocationRecord>> records;

void * FrameScopedGraphicsAllocator::allocate(mem::typeindex type, size_t count) {
    void* mem = allocators.local().arena.allocate(type, count);
    records.local().emplace_back(**type, count);
    return mem;
}

void FrameScopedGraphicsAllocator::reset() {
    for (auto& alloc : allocators) {
     //   std::cout << "Used: " << alloc.arena.bytes_used() << std::endl;
     //   std::cout << "Alloc: " << alloc.arena.total_capacity() << std::endl;
        alloc.arena.reset_compact();
    }
    size_t total = 0;

    for (auto& rcds : records) {

        for (auto& record : rcds) {
        //    std::cout << "Allocation : " << record.type.name << ": " << record.count << ", Bytes: " << record.type.size * record.count << std::endl;
            total += record.type.size * record.count;
        }
        rcds.clear();
    }
  //  std::cout << "Total: " << total << std::endl;
}

struct Renderer::Impl {
    ShaderComponentType shaderStorage{};
    GeometryResourceStorage geometryStorage{};
    BufferResourceStorage bufferResourceStorage;
    TextureResourceType textureStorage;
    MaterialStorage materialResource;


    ShaderKey fullScreenShader;
    GeometryKey fullScreenQuad;

    Impl(Renderer* renderer) : textureStorage(*renderer) {}
};

Renderer::Renderer(std::string_view name) : level(name.data()) {
    impl = new Impl(this);
    impl->materialResource.initialize(&impl->bufferResourceStorage, &impl->shaderStorage);
    level.addStage<RendererLoadResources>(this);
    level.addSystemEnumerable<RendererLoadResources, RenderPass>(this);
}

void Renderer::initialize() {
    impl->bufferResourceStorage.initialize();

    impl->fullScreenShader = impl->shaderStorage.createShader({
        .vertex = "shaders/screen/screen_vertex.glsl",
        .fragment = "shaders/screen/screen_frag.glsl"
    }, true);

    constexpr float quadVertices[] = {
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,

        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f
    };
    GeometryDescriptor quadDesc(quadVertices, VertexLayoutOf(VertexAttribute::Position2, VertexAttribute::TexCoord2));

    impl->fullScreenQuad = GeometryFactory(impl->geometryStorage).loadGeometry(quadDesc);
    synchronize();
    level.initialize();
    level.run<RendererLoadResources>();
    synchronize();
    std::cout << "Renderer initialized" << std::endl;
}


void Renderer::synchronize() {
    level.synchronize();
    impl->bufferResourceStorage.synchronizeGpuBuffers();
    impl->shaderStorage.synchronize();

    impl->textureStorage.synchronize();

    impl->geometryStorage.synchronize();
}

void Renderer::endFrame() {
    allocator.reset();
    impl->bufferResourceStorage.onFrameFinished();
    impl->materialResource.onFrameFinished(*this);
}

void Renderer::render(const RenderPass &renderPass) {
    GraphicsContext ctx(this);

    for (const auto& pass : renderPass.passes()) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, pass->name().data());

        pass->render(ctx);

        glPopDebugGroup();
    }
}

void Renderer::render(const RenderPass &renderPass, const RenderState &state, const RenderTexture &texture) {
    GraphicsContext ctx(this);
    ctx.setCurrentState(state);
    ctx.bindTextureForRendering(texture);

    for (const auto& pass : renderPass.passes()) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, pass->name().data());

        pass->render(ctx);

        glPopDebugGroup();
    }
}

const ShaderProgram * Renderer::getShaderProgram(const ShaderKey shader) const {
    return &impl->shaderStorage.getShader(shader);
}

ShaderComponentType & Renderer::getShaderStorage() {
    return impl->shaderStorage;
}

TextureResourceType & Renderer::getTextureStorage() {
    return impl->textureStorage;
}

GeometryResourceStorage & Renderer::getGeometryStorage() {
    return impl->geometryStorage;
}

BufferResourceStorage & Renderer::getBufferStorage() {
    return impl->bufferResourceStorage;
}

MaterialStorage & Renderer::getMaterialStorage() {
    return impl->materialResource;
}

ShaderKey Renderer::getFullScreenShader() {
    return impl->fullScreenShader;
}

GeometryKey Renderer::getFullScreenQuad() {
    return impl->fullScreenQuad;
}
