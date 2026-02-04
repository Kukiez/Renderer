#pragma once
#include <string_view>
#include <unordered_map>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <Image/Image.h>

#include "UIAPI.h"
#include "UIFont.h"

struct UIFontPixels {
    struct Glyph {
        void* ptr{};
        glm::ivec2 size{};
    };
    std::vector<Glyph> glyphs{};
};

struct UIFontLoadResult {
    UIFont font;
    UIFontPixels pixels;
    unsigned id;
};

struct GlyphMetrics {
    glm::ivec2 size{};
    glm::ivec2 bearing{};
    unsigned advance{};
    unsigned width{};
    unsigned height{};

    static constexpr auto isASCIIRenderable(char ascii) {
        return ascii >= 32 && ascii <= 126;
    }

    static constexpr auto ASCII_CHAR_BEGIN = 32;
    static constexpr auto ASCII_CHAR_END = 126;
};

class FontAsset {
    friend class UIFontLoader;

    std::vector<GlyphMetrics> glyphs;
    int resolution = 0;
public:
    FontAsset() = default;

    GlyphMetrics getGlyph(const wchar_t ch) const { return glyphs[ch]; }

    int getResolution() const { return resolution; }
};

struct FontRAsset {
    Image glyphImage{};
    std::vector<glm::vec4> glyphUVs;
};

class UIFontLoader {

public:
    UIAPI static std::pair<FontAsset, FontRAsset> cookFont(std::string_view path, std::string_view as, int resolution);

    UIAPI FontAsset loadFont(std::string_view font, int resolution);
};