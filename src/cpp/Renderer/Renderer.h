#pragma once
#include <ECS/Level/Level.h>

#include "Graphics/Features.h"
#include "Pipeline/PipelineRegistry.h"
#include "RenderingStages/LoadPass.h"
#include "Resource/Geometry/GeometryKey.h"
#include "Resource/Shader/ShaderKey.h"
#include "RendererAPI.h"

struct RenderState;
class BufferResourceStorage;
class GeometryResourceStorage;
class TextureResourceType;
class ShaderComponentType;
class ShaderProgram;
class MaterialStorage;
struct RenderTexture;
class RenderPass;
class RenderInvocation;
struct CameraComponent;

class FrameScopedGraphicsAllocator : public GraphicsAllocator {
    struct LocalArena {
        mem::byte_arena<mem::same_alloc_schema, 64> arena;

        struct AllocationTrack {
        };

        LocalArena() = default;
        LocalArena(size_t capacity) : arena(capacity) {}
    };
    tbb::enumerable_thread_specific<LocalArena> allocators;
public:
    FrameScopedGraphicsAllocator() : allocators(0.01 * 1024 * 1024) {}

    void* allocate(mem::typeindex type, size_t count) override;

    void reset();
};

class RENDERERAPI Renderer {
    struct Impl;

    size_t frameIndex = 0;
    FrameScopedGraphicsAllocator allocator;
    PipelineRegistry pipelineResourceStorage;
    Impl* impl{};
    Level level;
public:
    explicit Renderer(std::string_view name);

    template <typename T, typename... Args>
    const T* createPipeline(Args&&... args) {
        return pipelineResourceStorage.createPipeline<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    T* createPassPipeline(Args&&... args) {
        return pipelineResourceStorage.createPassPipeline<T>(std::forward<Args>(args)...);
    }

    void initialize();

    void synchronize();

    void endFrame();

    GraphicsAllocator* getRenderAllocator() {
        return &allocator;
    }

    void render(const RenderPass& pass);

    void render(const RenderPass& renderPass, const RenderState& state, const RenderTexture& texture);

    template <typename T>
    T& getResource() {
        return level.getSystem<T>();
    }

    template <typename T>
    T& getSystem() {
        return level.getSystem<T>();
    }

    template <typename T>
    T& addResource() {
        SystemAssembler<StageDetector<RendererLoadResources>> assembler(level.internal().systemRegistry);

        assembler.addSystem<T>();
        return level.getSystem<T>();
    }

    const ShaderProgram* getShaderProgram(ShaderKey shader) const;

    ShaderComponentType& getShaderStorage();
    TextureResourceType& getTextureStorage();
    GeometryResourceStorage& getGeometryStorage();
    BufferResourceStorage& getBufferStorage();
    MaterialStorage& getMaterialStorage();

    ShaderKey getFullScreenShader();
    GeometryKey getFullScreenQuad();
};