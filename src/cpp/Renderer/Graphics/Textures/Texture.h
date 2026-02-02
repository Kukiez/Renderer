#pragma once
#include <iosfwd>
#include <glm/vec4.hpp>

enum class PassBarrier {
    NONE = 0,
    SHADER_STORAGE = 1 << 0,
    COMMAND = 1 << 1,
    TEXTURE_FETCH = 1 << 2,
};

enum class TextureFormat : uint8_t {
    RGBA,
    RGB,
    RG,
    RED,
    RGBA_4,
    RGBA_8,
    RGB_8,
    RG_8,
    R_8,
    RGBA_16F,
    RGB_16F,
    RG_16F,
    R_16F,
    RGBA_32F,
    RGB_32F,
    RG_32F,
    R_32F,
    R_11F_G11F_B10F,
    RGBA8_SNORM,
    RGB8_SNORM,
    RGBA_16,
    RGB_16,
    DEPTH24_STENCIL8,
    DEPTH32F_STENCIL8,
    DEPTH_16F,
    DEPTH_32F
};

enum class TextureMinFilter : uint8_t {
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_NEAREST,
    LINEAR_MIPMAP_LINEAR
};

enum class TextureMagFilter : uint8_t {
    NEAREST = 0,
    LINEAR  = 1
};

enum class TextureWrap : uint8_t {
    REPEAT = 0,
    MIRRORED_REPEAT = 1,
    EDGE_CLAMP = 2,
    BORDER_CLAMP = 3,
};


enum class TextureCompareMode : uint8_t {
    NONE,
    REF_TO_TEXTURE
};

enum class TextureCompareFunc : uint8_t {
    LESS,
    LEQUAL,
    GEQUAL,
    GREATER
};




enum class TextureTarget : uint8_t {
    INVALID,

    TEXTURE_1D,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D,
    TEXTURE_2D_MSAA,
    TEXTURE_ARRAY_2D,
    CUBEMAP,
    TEXTURE_3D,
};

std::ostream& operator<<(std::ostream& os, const TextureTarget& target);

enum class Mipmap : uint8_t {
    AUTO = 255,
    NONE = 0,
    ONE = 1,
    TWO = 2,
    THREE = 3,
    FOUR = 4,
    FIVE = 5,
    SIX = 6,
    SEVEN = 7,
    EIGHT = 8,
    SIXTEEN = 16,
};

enum class Anisotropy {
    AUTO = -1,
    NONE = 0
};

struct TextureBorderColor {
    glm::vec4 color = glm::vec4(0.f);

    constexpr TextureBorderColor() = default;

    constexpr TextureBorderColor(const glm::vec4 color) : color(color) {}
    constexpr TextureBorderColor(const float r, const float g, const float b, const float a) : color(r, g, b, a) {}
};

struct TextureCreateParams {
    TextureFormat format = TextureFormat::RGBA_8;
    TextureMinFilter minFilter = TextureMinFilter::LINEAR;
    TextureMagFilter magFilter = TextureMagFilter::LINEAR;
    TextureWrap wrap = TextureWrap::REPEAT;
    Mipmap mipmap = Mipmap::NONE;
    Anisotropy anisotropy = Anisotropy::NONE;

    TextureCompareMode compareMode = TextureCompareMode::NONE;
    TextureCompareFunc compareFunc = TextureCompareFunc::LEQUAL;

    TextureBorderColor border = {};

    float lodBias = 0.f;
    unsigned arrayIndices = 1;

    TextureCreateParams() = default;

    constexpr TextureCreateParams(const TextureFormat format, const Mipmap mipmap, const Anisotropy anisotropy = Anisotropy::AUTO) : format(format), mipmap(mipmap), anisotropy(anisotropy) {
        if (mipmap == Mipmap::AUTO) {
            minFilter = TextureMinFilter::LINEAR_MIPMAP_LINEAR;
            magFilter = TextureMagFilter::LINEAR;
        }
    }

    constexpr TextureCreateParams(const TextureFormat format, const TextureMinFilter minFilter,
        const TextureMagFilter magFilter, const TextureWrap wrap, const Mipmap mipmap,
        const Anisotropy anisotropy = Anisotropy::AUTO, const float lodBias = 0.f,
        const unsigned arrayIndices = 1, TextureCompareMode compareMode = TextureCompareMode::NONE,
        TextureCompareFunc compareFunc = TextureCompareFunc::LEQUAL, TextureBorderColor border = {}
        )
    : format(format), minFilter(minFilter), magFilter(magFilter),
    wrap(wrap), mipmap(mipmap), anisotropy(anisotropy),
    compareMode(compareMode), compareFunc(compareFunc), lodBias(lodBias), border(border)
    {}
};

struct TexturePreset {
    static constexpr auto SHADOW_CUBEMAP_32F = TextureCreateParams(
        TextureFormat::DEPTH_32F, TextureMinFilter::NEAREST, TextureMagFilter::NEAREST, TextureWrap::EDGE_CLAMP,
        Mipmap::NONE, Anisotropy::NONE, 0.f, 1,
        TextureCompareMode::REF_TO_TEXTURE, TextureCompareFunc::LEQUAL
    );

    static constexpr auto COLOR_RGBA8 = TextureCreateParams(
        TextureFormat::RGBA_8,
        TextureMinFilter::LINEAR,
        TextureMagFilter::LINEAR,
        TextureWrap:: EDGE_CLAMP,
        Mipmap::NONE,
        Anisotropy::NONE,
        0.0f,
        1,
        TextureCompareMode::NONE,
        TextureCompareFunc::LESS
    );

    static constexpr auto SHADOW_2D_32F = TextureCreateParams(
        TextureFormat::DEPTH_32F,
        TextureMinFilter::NEAREST,
        TextureMagFilter::NEAREST,
        TextureWrap::EDGE_CLAMP,
        Mipmap::NONE,
        Anisotropy::NONE,
        0.f,
        1,
        TextureCompareMode::NONE,
        TextureCompareFunc::LESS,
        TextureBorderColor(1, 1, 1, 1)
    );

    static constexpr auto DEPTH_BUFFER_32F = TextureCreateParams(TextureFormat::DEPTH_32F, {}, {});

    static constexpr auto COLOR_BUFFER_RGBA16F = TextureCreateParams(TextureFormat::RGBA_16F, {}, {});
};


static constexpr auto PBR_DIFFUSE_TEXTURE_RGB = TextureCreateParams(
    TextureFormat::RGB_8, TextureMinFilter::LINEAR_MIPMAP_LINEAR, TextureMagFilter::LINEAR, TextureWrap::REPEAT, Mipmap::AUTO, Anisotropy::AUTO
);

static constexpr auto PBR_NORMAL_TEXTURE_RGB = TextureCreateParams(
    TextureFormat::RGB_8, TextureMinFilter::LINEAR_MIPMAP_LINEAR, TextureMagFilter::LINEAR, TextureWrap::REPEAT, Mipmap::AUTO, Anisotropy::AUTO
);


static constexpr auto PBR_DISPLACEMENT_TEXTURE_8BIT = TextureCreateParams(
    TextureFormat::R_8, TextureMinFilter::LINEAR_MIPMAP_LINEAR, TextureMagFilter::LINEAR, TextureWrap::REPEAT, Mipmap::AUTO, Anisotropy::AUTO
);

static constexpr auto PBR_AO_ROUGH_METALLIC_EXR = TextureCreateParams(
    TextureFormat::RGB_16F, TextureMinFilter::LINEAR_MIPMAP_LINEAR, TextureMagFilter::LINEAR, TextureWrap::REPEAT, Mipmap::AUTO, Anisotropy::AUTO
);

enum class NullTexture_t {};

static constexpr NullTexture_t nulltexture{};

struct TextureBase {
protected:
    int myWidth = 0;
    int myHeight = 0;
    int myDepth = 0;

    unsigned texture = 0;
public:
    TextureBase() = default;

    TextureBase(const int width, const int height, const int depth = 1) : myWidth(width), myHeight(height), myDepth(depth) {}

    int width() const { return myWidth; }
    int height() const { return myHeight; }
    int depth() const { return myDepth; }

    void gen(const TextureTarget target);

    void discard();

    unsigned id() const { return texture; }

    bool operator == (const NullTexture_t&) const {
        return texture == 0;
    }

    bool operator != (const NullTexture_t&) const {
        return texture != 0;
    }
};