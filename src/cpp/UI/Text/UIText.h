#pragma once

namespace ui {
    struct UITextStyle {
        struct Font {
            unsigned size = 16;
        };
        struct Shadow {
            glm::vec2 offset{};
            glm::vec3 color{};
        };
        glm::vec3 color{};
        float opacity = 1.f;
        Font font{};
        Shadow shadow{};
    };

    struct UITextRun {
        const UITextStyle* style{};
        unsigned textStart{};
        unsigned textLength{};

        std::string_view getText(std::string_view fullText) const {
            if (fullText.empty()) return {};

            size_t from = std::min(static_cast<size_t>(textStart), fullText.size() - 1);
            size_t to = std::min(from + textLength, fullText.size());

            return fullText.substr(from, to);
        }
    };

    class UITextStream {
        std::string text{};
    public:
        UITextStream() = default;

        std::string_view getText() const { return text; }
        size_t size() const { return text.size(); }

        UITextStream& operator += (const std::string_view& str) { text += str; return *this; }
    };

    class UITextLayout {
        std::vector<UITextRun> runs{};
    public:
        UITextLayout& addRun(size_t first, size_t size, const UITextStyle* style) {
            auto start = static_cast<unsigned>(first);
            runs.emplace_back(UITextRun{style, start, static_cast<unsigned>(size)});
            return *this;
        }

        auto& getRuns() const { return runs; }
    };
}