#pragma once


class FileSerializer {
    struct FileSection {
        std::string_view name;

    };
public:
    static std::string_view getSection(std::string_view code, std::string_view block) {
        auto passesBegin = code.find(block);

        if (passesBegin == std::string::npos) {
            return "";
        }
        passesBegin += block.length() + 2;

        size_t count = 0;

        size_t openBracket = 0;

        while (code[passesBegin + count] != '}' && openBracket == 0) {
            if (code[passesBegin + count] == '{') {
                ++openBracket;
            }
            count++;

            if (count == code.size()) {
                return "";
            }
        }

        std::string_view passes = code.substr(passesBegin, count);
        return passes;
    }
};