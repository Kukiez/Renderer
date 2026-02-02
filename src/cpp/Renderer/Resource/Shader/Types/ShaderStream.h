#pragma once
#include <charconv>
#include <string>

struct ShaderValueConstructor;

class ShaderStream {
    std::string& result;
public:
    ShaderStream(std::string& result) : result(result) {}

    void reserve(const size_t size) {
        result.reserve(size);
    }

    ShaderStream& operator << (const char* str) {
        result.append(str);
        return *this;
    }

    ShaderStream& operator<<(float f) {
        char buf[32];

        auto end = std::to_chars(buf, buf + sizeof(buf), f).ptr;

        result.append(buf, end - buf);
        return *this;
    }

    void addAssignmentLine(std::string_view variable, const ShaderValueConstructor& value);

    ShaderStream& operator<<(const std::string_view & string_view);
};
