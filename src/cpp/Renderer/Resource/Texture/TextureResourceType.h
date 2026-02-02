#pragma once
#include <ECS/ThreadLocal.h>
#include <ECS/Component/Component.h>
#include <openGL/BufferObjects/FrameBufferObject.h>
#include <Renderer/Graphics/GraphicsContext.h>
#include <Renderer/Graphics/Textures/Texture2D.h>
#include <Renderer/Graphics/Textures/Texture2DMS.h>
#include <Renderer/Graphics/Textures/Cubemap.h>
#include <Renderer/Resource/MultiThreadedKeyGenerator.h>

#include "TextureDescriptor.h"
#include "RenderTexture.h"
#include "TextureKey.h"

class TextureAsset;
struct LevelContext;

struct TextureResourceStagingBuffer {
    struct Ops {
        std::vector<std::pair<TextureKey, TextureDescriptor2D>> createTexture;
        std::vector<std::pair<TextureKey, EmptyCubemapDescriptor2D>> createEmptyCubemap;
        std::vector<std::pair<TextureKey, TextureDescriptor2DArray>> createTextureArray2D;
    };

    ThreadLocal<Ops> ops;
    tbb::concurrent_vector<std::pair<TextureKey, TextureDescriptor2DMS>> texture2DMS;

    size_t lengthTex2D() {
        size_t len = 0;
        for (auto& op : ops) {
            len += op.createTexture.size();
        }
        return len;
    }

    size_t lengthCubemap() {
        size_t len = 0;
        for (auto& op : ops) {
            len += op.createEmptyCubemap.size();
        }
        return len;
    }
};

struct FramebufferAttachmentDescriptor {
    FramebufferAttachmentCreateParams attachmentParams;
    int width;
    int height;
};

struct FramebufferEntry {
    FrameBufferObject framebuffer;

    operator FrameBufferObject&() { return framebuffer; }
};

class TextureResourceType {
    friend class TextureFactory;
    friend class TextureQuery;
    friend class TextureSynchronousFactory;

    MultiThreadedKeyGenerator<TextureKey> generator{};

    struct Texture2DResource {
        Texture2D texture{};
        TextureCreateParams params{};

        Texture2DResource() {

        }

        Texture2DResource(Texture2D&& texture, TextureCreateParams params) : texture(std::move(texture)), params(std::move(params)) {}
    };

    struct Texture3DResource {

    };

    struct CubemapResource {
        Cubemap cubemap;
        TextureCreateParams params;
    };

    struct Texture1DArrayResource {

    };

    struct Texture2DArrayResource {
        Texture2DArray texture;
    };

    struct Texture2DMSResource {
        Texture2DMS texture;
    };

    struct TextureResource {
        unsigned resID = 0;
        size_t handle = 0;
    };

    struct TextureAssetPointer {
        TextureKey key{};
    };

    size_t makeTextureResident(unsigned textureGpuId, TextureKey key);

    Texture2D& createTexture2D(TextureKey texture, TextureDescriptor2D& descriptor);
    Texture2DMS& createTexture2DMS(TextureKey texture, TextureDescriptor2DMS& descriptor);
    Texture2DArray& createTextureArray2D(TextureKey texture, TextureDescriptor2DArray& descriptor);

    Cubemap &createEmptyCubemap(TextureKey key, const EmptyCubemapDescriptor2D &descriptor);

    std::vector<Texture2DResource> textures2D;
    std::vector<Texture2DArrayResource> textures2DArray;
    std::vector<CubemapResource> cubemaps;
    std::vector<Texture2DMSResource> textures2DMS;

    std::vector<TextureResource> allTextures;

    std::vector<TextureAssetPointer> cpuToGpu;

    TextureResourceStagingBuffer stagingBuffer;

    std::unordered_map<RenderTexture, FramebufferEntry, RenderTextureHash, RenderTextureEq> textureFramebuffers;
    std::unordered_map<MultiRenderTexture, FramebufferEntry, MultiRenderTextureHash> multiRenderTextureFramebuffers;

    bool firstSync = true;

    BufferKey resource2DMaterialBuffer{};

    Renderer* renderer;
public:
    TextureResourceType(Renderer& renderer);

    void synchronize();

    TextureKey newTexture(TextureTarget target) {
        return generator.generate(target);
    }

    FramebufferAttachmentDescriptor createAttachmentParams(const RenderTexture &renderTexture) const;

    FrameBufferObject& createFramebuffer(const RenderTexture& renderTexture);
    FrameBufferObject& createFramebuffer(const MultiRenderTexture& renderTexture);

    unsigned getResourceID(const TextureKey tex) const {
        return allTextures[tex.index()].resID;
    }
};