#pragma once

class UIFont {
public:
    struct Character {
        unsigned int ID;      // Layer ID
        glm::ivec2 size;      // Size of the glyph
        glm::ivec2 bearing;   // Offset from baseline to left/top of glyph
        unsigned int advance; // Horizontal offset to advance to the next glyph
    };
private:
    friend class UIFontLoader;

    std::vector<Character> characters{};

    int charResolution = 0;
public:
    UIFont() = default;
    UIFont(std::vector<Character>&& characters, const int resolution) : characters(std::move(characters)), charResolution(resolution) {}

    const Character& operator [] (const wchar_t ch) const {
        return characters[ch];
    }

    auto begin() const {
        return characters.begin();
    }

    auto end() const {
            return characters.end();
    }

    int resolution() const {
        return charResolution;
    }
};
