#pragma once
#include <util/functions.h>

struct Color {
    constexpr static glm::vec4 hexToColor(const uint32_t rgba) {
        const float r = ((rgba >> 24) & 0xFF) / 255.0f;
        const float g = ((rgba >> 16) & 0xFF) / 255.0f;
        const float b = ((rgba >> 8)  & 0xFF) / 255.0f;
        const float a = (rgba         & 0xFF) / 255.0f;
        return glm::vec4(r, g, b, a);
    }

    constexpr static glm::vec3 xyz(const Color color) {
        return glm::vec3(color.r, color.g, color.b);
    }

    static Color fromRGBA(const std::string& hex) {
        const std::string hexPart = hex[0] == '#' ? hex.substr(1) : hex;
        uint32_t color = 0;
        std::stringstream ss(hexPart);
        ss >> std::hex >> color;

        if (hexPart.size() == 6) {
            return Color(
                ((color >> 16) & 0xFF) / 255.0f,
                ((color >> 8)  & 0xFF) / 255.0f,
                ((color >> 0)  & 0xFF) / 255.0f,
                1.0f
            );
        }
        if (hexPart.size() == 8) {
            return Color(
                ((color >> 24) & 0xFF) / 255.0f,
                ((color >> 16) & 0xFF) / 255.0f,
                ((color >> 8)  & 0xFF) / 255.0f,
                ((color >> 0)  & 0xFF) / 255.0f
            );
        }
        return Color(1.0f, 0.0f, 1.0f, 1.0f);
    }

    enum ConstantColor : uint32_t {
        RED = 0xFF0000FF,
        GREEN = 0x00FF00FF,
        BLUE = 0x007BFFFF,
        DARK_GRAY = 0xFF808080,
        GRAY = 0xFF808080,
        WHITE = 0xFFFFFFFF,
        BLACK = 0x000000FF,
        MAGENTA = 0xFF00FF00,
        YELLOW = 0xFFFF00FF,
        ORANGE = 0xFF8C00,
        PURPLE = 0xFF00FF,
        PINK = 0xFF7FFFFF,
        AQUA = 0x00FFFFFF,
        CYAN = 0x00FFFFFF,
        DARK_BLUE = 0xFF0000FF,
        DARK_GREEN = 0xFF00FF,
        DARK_CYAN = 0xFF00FF,
        DARK_RED = 0xFF00FF,
        DARK_MAGENTA = 0xFF00FF,
        DARK_YELLOW = 0xFF00FF,
    };

    float r,g,b,a;

    constexpr Color(const float r, const float g, const float b, const float a) : r(r), g(g), b(b), a(a) {}
    explicit constexpr Color(const float scalar) : r(scalar), g(scalar), b(scalar), a(scalar) {}
    constexpr Color(ConstantColor constant) {
        glm::vec4 rgba = hexToColor(constant);

        r = rgba[0]; g = rgba[1]; b = rgba[2]; a = rgba[3];
    }

    constexpr Color(const glm::vec4 color) : r(color[0]), g(color[1]), b(color[2]), a(color[3]) {}

    Color() : Color(0, 0, 0, 0) {}

    Color(const std::string &str) {
        const glm::vec4 rgba = fromRGBA(str);

        memcpy(this, &rgba, sizeof(rgba));
    }

    operator glm::vec4() const {
        return glm::vec4(r, g, b, a);
    }

    Color operator+(const Color &other) const {
        return Color(r + other.r, g + other.g, b + other.b, a + other.a);
    }

    Color operator-(const Color &other) const {
        return Color(r - other.r, g - other.g, b - other.b, a - other.a);
    }


    Color operator * (const Color &other) const {
        return Color(r * other.r, g * other.g, b * other.b, a * other.a);
    }

    Color operator/(const Color &other) const {
        return Color(r / other.r, g / other.g, b / other.b, a / other.a);
    }

    Color& operator += (const glm::vec4& color) {
        r += color[0]; g += color[1]; b += color[2]; a += color[3];
        return *this;
    }

    Color& operator *= (const glm::vec4& color) {
        r *= color[0]; g *= color[1]; b *= color[2]; a *= color[3];
        return *this;
    }

    Color& operator = (const glm::vec4& color) {
        memcpy(this, &color, sizeof(color));
        return *this;
    }

    Color operator * (const double scalar) const {
        return Color(r * static_cast<float>(scalar), g * static_cast<float>(scalar), b * static_cast<float>(scalar), a * static_cast<float>(scalar));
    }

    friend std::ostream& operator<<(std::ostream &os, const Color &color) {
        os << color.r << ", " << color.g << ", " << color.b << ", " << color.a;
        return os;
    }

    Color& operator *= (const Color &other) {
        r *= other.r; g *= other.g; b *= other.b; a *= other.a;
        return *this;
    }

    Color& operator += (const Color& other) {
        r += other.r; g += other.g; b += other.b; a += other.a;
        return *this;
    }

    Color& operator /= (const Color &other) {
        r /= other.r; g /= other.g; b /= other.b, a /= other.a;
        return *this;
    }

    Color& operator /= (const double scalar) {
        r /= static_cast<float>(scalar); g /= static_cast<float>(scalar); b /= static_cast<float>(scalar); a /= static_cast<float>(scalar);
        return *this;
    }

    Color& operator += (const double scalar) {
        r += static_cast<float>(scalar); g += static_cast<float>(scalar); b += static_cast<float>(scalar); a += static_cast<float>(scalar);
        return *this;
    }

    Color& operator += (const float scalar) {
        r += static_cast<float>(scalar); g += static_cast<float>(scalar); b += static_cast<float>(scalar); a += static_cast<float>(scalar);
        return *this;
    }


    Color fmod() {
        return Color(::fmod(r, 1.0f), ::fmod(g, 1.0f), ::fmod(b, 1.0f), ::fmod(a, 1.0f));
    }
};
