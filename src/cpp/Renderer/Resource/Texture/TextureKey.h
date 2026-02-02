#pragma once
#include <Renderer/Graphics/Textures/Texture.h>

#include "../ResourceKey.h"

class SamplerKey;

class TextureKey {
    unsigned myID : 24;
    unsigned target : 8;
public:
    constexpr TextureKey() : myID(0), target(0) {
        static_assert(sizeof(TextureKey) == sizeof(unsigned), "TextureKey is not equal to sizeof(unsigned)");
    }

    template <std::integral T>
    constexpr TextureKey(const T key, TextureTarget target) : myID(static_cast<unsigned>(key)), target(static_cast<unsigned>(target)) {}

    constexpr TextureTarget type() const {
        return static_cast<TextureTarget>(target);
    }

    size_t hash() const {
        return std::hash<unsigned>{}(myID);
    }

    bool operator==(const TextureKey & other) const {
        return myID == other.myID;
    }

    bool operator!=(const TextureKey & other) const {
        return !(*this == other);
    }

    constexpr unsigned id() const {
        return myID;
    }

    constexpr unsigned index() const {
        return myID;
    }

    bool isValid() const {
        return myID != 0;
    }

    SamplerKey sampler() const;
};

static constexpr auto NULL_TEXTURE_KEY = TextureKey{};
static constexpr auto DEFAULT_FRAMEBUFFER_SCREEN_TEXTURE_KEY = TextureKey{std::numeric_limits<unsigned>::max() - 1, TextureTarget::TEXTURE_2D};

class SamplerKey {
    TextureKey texture{};
    unsigned samplerID : 31;
    unsigned isDefaultTextureKey : 1;
public:
    constexpr SamplerKey() : samplerID(0), isDefaultTextureKey(0) {}

    SamplerKey(const TextureKey& tex) : texture(tex), samplerID(0), isDefaultTextureKey(1) {}

    bool isTextureDefault() const { return isDefaultTextureKey; }

    TextureKey getTexture() const { return texture; }

    unsigned id() const {
        return samplerID;
    }

    bool operator==(const SamplerKey & other) const {
        return texture == other.texture && samplerID == other.samplerID;
    }

    bool operator!=(const SamplerKey & other) const {
        return !(*this == other);
    }
};

inline SamplerKey TextureKey::sampler() const {
    return SamplerKey{*this};
}

static constexpr auto NULL_SAMPLER_KEY = SamplerKey{};