#pragma once
#include <openGL/BufferObjects/FrameBufferObject.h>
#include <Renderer/Resource/Texture/TextureKey.h>

struct RenderTexture {
    TextureKey texture{};
    CubemapFace face{};
    Mipmap mipmap{};
    FramebufferAttachmentIndex attachment{};
    unsigned layer{};

    static RenderTexture DefaultScreen() {
        return RenderTexture{DEFAULT_FRAMEBUFFER_SCREEN_TEXTURE_KEY};
    }

    static RenderTexture Depth(TextureKey depthTexture, CubemapFace face = {}, Mipmap mipmap = {}, unsigned layer = 0) {
        return RenderTexture{depthTexture, face, mipmap, FramebufferAttachmentIndex::DEPTH, layer};
    }

    bool operator==(const RenderTexture& other) const noexcept {
        return texture == other.texture && mipmap == other.mipmap && face == other.face && layer == other.layer && attachment == other.attachment;
    }

    bool isColorAttachment() const noexcept {
        const int attachIndex = static_cast<int>(attachment);

        if (attachIndex < 0) return false;
        if (attachIndex >= 0 && attachIndex < 8) return true;
        return false;
    }

    bool isDepthAttachment() const noexcept {
        return attachment == FramebufferAttachmentIndex::DEPTH;
    }

    bool isStencilAttachment() const noexcept {
        return attachment == FramebufferAttachmentIndex::STENCIL;
    }

    bool isDepthStencilAttachment() const noexcept {
        return attachment == FramebufferAttachmentIndex::DEPTH_STENCIL;
    }
};

struct RenderTextureHash {
    std::size_t operator()(const RenderTexture& r) const noexcept {
        std::size_t h = 0;
        h ^= r.texture.hash();
        h ^= std::hash<unsigned>()(static_cast<unsigned>(r.mipmap)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<unsigned>()(static_cast<unsigned>(r.face)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<unsigned>()(r.layer) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<unsigned>()(static_cast<unsigned>(r.attachment)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

struct RenderTextureEq {
    bool operator()(const RenderTexture& a, const RenderTexture& b) const noexcept {
        return a.texture == b.texture && a.mipmap == b.mipmap &&
               a.face   == b.face &&
               a.layer  == b.layer && a.attachment == b.attachment;
    }
};

struct MultiRenderTexture {
    RenderTexture textures[8]{};
    unsigned numTextures{};

    MultiRenderTexture() = default;

    MultiRenderTexture(const RenderTexture& texture) {
        textures[0] = texture;
        numTextures = 1;
    }

    MultiRenderTexture(const RenderTexture& texture1, const RenderTexture& texture2) {
        textures[0] = texture1;
        textures[1] = texture2;
        numTextures = 2;

        std::ranges::sort(mem::make_range(this->textures, 2), RenderTextureEq{});
    }

    MultiRenderTexture(const std::initializer_list<RenderTexture> textures) {
        assert(textures.size() <= 8);

        size_t i = 0;

        for (auto& texture : textures) {
            this->textures[i++] = texture;

            if (i == 8) break;
        }
        numTextures = std::min(textures.size(), 8ull);

        std::ranges::sort(mem::make_range(this->textures, numTextures), RenderTextureEq{});
    }

    bool operator==(const MultiRenderTexture & other) const {
        if (numTextures != other.numTextures) return false;
        for (size_t i = 0; i < numTextures; ++i) {
            if (textures[i] != other.textures[i]) return false;
        }
        return true;
    }

    const RenderTexture& operator[](const size_t index) const {
        assert(index < numTextures);
        return textures[index];
    }

    auto targets() const { return mem::make_range(textures, numTextures); }

    bool empty() const { return numTextures == 0; }

    size_t size() const { return numTextures; }
};

struct MultiRenderTextureHash {
    std::size_t operator()(const MultiRenderTexture& r) const noexcept {
        std::size_t h = 0;
        for (const auto& texture : mem::make_range(r.textures, r.numTextures)) {
            h ^= RenderTextureHash{}(texture);
        }
        return h;
    }
};

class RenderTextureView {
    union {
        struct {
            RenderTexture texture;
        };
        struct {
            const RenderTexture* texturePtr;
            size_t numTextures;
        };
    };
    bool isMulti = false;
public:
    RenderTextureView() {
        texture = RenderTexture{};
    }

    RenderTextureView(const RenderTexture& texture) {
        this->texture = texture;
        this->isMulti = false;
    }

    RenderTextureView(const MultiRenderTexture& textures) {
        this->texturePtr = textures.textures;
        this->numTextures = textures.numTextures;
        this->isMulti = true;
    }
};