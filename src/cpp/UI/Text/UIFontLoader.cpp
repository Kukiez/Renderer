
#include "UIFontLoader.h"

#include <fstream>
#include <ft2build.h>
#include <string>
#include <stb/stb_image_write.h>
#include <Filesystem/Serializer.h>
#include <Image/ImageLoader.h>

#include FT_FREETYPE_H

uint32_t nextPow2(uint32_t v) {
    if (v == 0) return 1;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}

uint32_t chooseAtlasSize(uint32_t totalPixelsNeeded) {
    uint32_t minSide =
        static_cast<uint32_t>(std::ceil(std::sqrt(totalPixelsNeeded)));

    return nextPow2(minSide);
}

std::pair<FontAsset, FontRAsset> UIFontLoader::cookFont(std::string_view path, std::string_view as, int resolution) {

    FT_Library ft;

    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Failed to init FreeType Library\n";
        std::exit(-1);
    }

    FT_Face face;

    if (FT_New_Face(ft, path.data(), 0, &face)) {
        std::cerr << "Failed to load font\n";
    }

    FT_Set_Pixel_Sizes(face, 0, resolution);


    std::vector<GlyphMetrics> glyphs(128);
    unsigned char* glyphsPixelData[128]{};

    unsigned totalPixelsNeeded = 0;

    for (unsigned char c = GlyphMetrics::ASCII_CHAR_BEGIN; c < GlyphMetrics::ASCII_CHAR_END; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load Glyph: " << c << std::endl;
            continue;
        }

        unsigned width = face->glyph->bitmap.width;
        unsigned height = face->glyph->bitmap.rows;
        int pitch = face->glyph->bitmap.pitch;

        auto bitmapData = face->glyph->bitmap.buffer;

        auto size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
        auto bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
        auto advance = static_cast<unsigned>(face->glyph->advance.x >> 6);

        auto glyphData = new unsigned char[width * height];

        if (pitch < 0) {
            bitmapData += (-pitch) * (height - 1);
            pitch = -pitch;
        }

        for (unsigned y = 0; y < height; ++y) {
            std::memcpy(
                glyphData + y * width,
                bitmapData + y * pitch,
                width
            );
            assert(glyphData + y * width <= glyphData + width * height);
        }

        glyphs[c] = {size, bearing, advance, width, height};
        glyphsPixelData[c] = glyphData;

        totalPixelsNeeded += width * height;
    }
    unsigned atlasSize = chooseAtlasSize(totalPixelsNeeded);
    Image fontImg = ImageLoader::procedural(atlasSize, atlasSize, PixelType::UNSIGNED_BYTE, ImageChannels::R);

    std::vector<glm::vec4> glyphUVs;
    glyphUVs.resize(128);

    unsigned u = 0;
    unsigned v = 0;

    unsigned maxRowHeight = 0;

    static constexpr auto PADDING_PIXELS = 20;

    for (unsigned char c = GlyphMetrics::ASCII_CHAR_BEGIN; c < GlyphMetrics::ASCII_CHAR_END; c++) {
        auto& glyph = glyphs[c];

        if (u + glyph.width > atlasSize) {
            u = 0;
            v += maxRowHeight + PADDING_PIXELS;
            maxRowHeight = 0;
        }

        for (unsigned w = 0; w < glyph.width; ++w) {
            for (unsigned h = 0; h < glyph.height; ++h) {
                DynamicPixel pixel{};
                unsigned currIdx = w + h * glyph.width;
                std::memcpy(pixel.r, glyphsPixelData[c] + currIdx, 1);
                fontImg.store(u + w, v + h, pixel);
            }
        }

        maxRowHeight = std::max(maxRowHeight, glyph.height);

        glm::vec4 uv = {u, v, u + glyph.width, v + glyph.height};
        uv /= glm::vec4(atlasSize, atlasSize, atlasSize, atlasSize);

        glyphUVs[c] = uv;
        u += glyph.width + PADDING_PIXELS;
    }

    for (auto& glyph : glyphsPixelData) {
        delete[] glyph;
    }

    std::string fontDesc{};
    fontDesc.reserve(12000);

    for (unsigned char c = GlyphMetrics::ASCII_CHAR_BEGIN; c < GlyphMetrics::ASCII_CHAR_END; c++) {
        fontDesc += c;

        fontDesc += " {\n";
        fontDesc += "width: " + std::to_string(glyphs[c].width) + ",\n";
        fontDesc += "height: " + std::to_string(glyphs[c].height) + ",\n";
        fontDesc += "bearing: [" + std::to_string(glyphs[c].bearing.x) + ", " + std::to_string(glyphs[c].bearing.y) + "],\n";
        fontDesc += "advance: " + std::to_string(glyphs[c].advance) + "\n";
        fontDesc += "}\n";
    }


    std::string fontBasePath(as);
    std::string fontImgPath = fontBasePath + ".png";
    std::string fontDescPath = fontBasePath + ".txt";

    ImageLoader::save(fontImg, fontImgPath);

    std::ofstream fontFile(fontDescPath, std::ios::trunc | std::ios::out, std::ios::binary);

    if (fontFile.is_open()) {
        fontFile.write(fontDesc.data(), fontDesc.size());
    } else {
        assert(false);
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    FontAsset fontAsset{};
    fontAsset.glyphs = std::move(glyphs);
    fontAsset.resolution = resolution;

    FontRAsset fontRAsset{};
    fontRAsset.glyphUVs = std::move(glyphUVs);
    fontRAsset.glyphImage = std::move(fontImg);

    return {
        std::move(fontAsset), std::move(fontRAsset)
    };
}

FontAsset UIFontLoader::loadFont(std::string_view font, int resolution) {
    std::string fontBasePath(font);
    std::string fontImgPath = fontBasePath + ".png";
    std::string fontDescPath = fontBasePath + ".txt";

    std::fstream fontFile(fontDescPath, std::ios::in | std::ios::binary);

    if (!fontFile.is_open()) {
        assert(false);
    } else {
        std::string fontDesc((std::istreambuf_iterator<char>(fontFile)), std::istreambuf_iterator<char>());

        std::string_view rest = fontDesc;

        for (unsigned char c = 0; c < 128; c++) {
            char ch = static_cast<char>(c);

            auto chSection = FileSerializer::getSection(rest, &ch);
        }
    }
    return {};
}

