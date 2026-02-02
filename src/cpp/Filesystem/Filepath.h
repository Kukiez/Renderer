#pragma once
#include <string_view>
#include <iostream>
#include <cassert>

template <int MaxLength>
class BasicFilepath {
public:
    static constexpr auto MAX_FILE_LENGTH = MaxLength;

    using LengthType = size_t;
private:
    template <typename... Strs>
    static void error(Strs&&... strings) {
        size_t length = (strings.length() + ...);

        std::cerr << "Filepath too long: ";
        ((std::cerr << strings), ...);
        std::cerr << " with length: " << length << ", max: " << MAX_FILE_LENGTH << std::endl;
        assert(false);
    }

    char filepath[MAX_FILE_LENGTH]{};
    LengthType length{};
public:
    BasicFilepath() = default;

    constexpr BasicFilepath(std::string_view filepath) {
        if (filepath.length() > MAX_FILE_LENGTH) {
            error(filepath);
        }
        for (size_t i = 0; i < filepath.length(); ++i) {
            this->filepath[i] = filepath[i];
        }
        this->filepath[filepath.length()] = '\0';
        length = static_cast<LengthType>(filepath.length());
    }

    template <int N>
    constexpr BasicFilepath(const BasicFilepath<N>& other) {
        *this = other.path();
    }

    constexpr BasicFilepath(const char* filepath) : BasicFilepath(std::string_view(filepath)) {}

    const char* data() const { return filepath; }

    std::string_view path() const { return { filepath, length }; }

    operator std::string_view() const { return path(); }

    std::string_view filename() const {
        size_t slash = path().find_last_of("/\\");

        return (slash == std::string_view::npos)
            ? path()
            : path().substr(slash + 1);
    }

    std::string_view stem() const {
        auto filename = this->filename();
        size_t dot = filename.find_last_of('.');
        return (dot == std::string_view::npos)
            ? filename
        : filename.substr(0, dot);
    }

    std::string_view extension() const {
        auto filename = this->filename();
        size_t dot = filename.find_last_of('.');
        return (dot == std::string_view::npos || dot == 0)
            ? std::string_view{}
        : filename.substr(dot);
    }

    std::string_view parentPath() const {
        size_t slash = path().find_last_of("/\\");
        return (slash == std::string_view::npos)
        ? std::string_view{}
        : path().substr(0, slash);
    }

    BasicFilepath operator + (const std::string_view& other) const {
        if (length + other.length() >= MAX_FILE_LENGTH) {
            error(path(), other);
        }
        BasicFilepath result;
        std::memcpy(result.filepath, filepath, length);
        std::memcpy(result.filepath + length, other.data(), other.length());
        result.filepath[length + other.length()] = '\0';
        result.length = static_cast<LengthType>(length + other.length());
        return result;
    }

    bool operator == (const BasicFilepath& other) const { return path() == other.path(); }
    BasicFilepath operator + (const char* other) const { return *this + std::string_view(other); }
    BasicFilepath operator + (char other) const { return *this + std::string_view(&other, 1); }

    BasicFilepath operator + (const BasicFilepath& other) const { return *this + other.path(); }

    BasicFilepath& operator += (const std::string_view& other) { return *this = *this + other; }
    BasicFilepath& operator += (const char* other) { return *this += std::string_view(other); }
    BasicFilepath& operator += (char other) { return *this += std::string_view(&other, 1); }

    template <int N>
    BasicFilepath& operator += (const BasicFilepath<N>& other) { return *this += other.path(); }

    friend std::ostream& operator<<(std::ostream& os, const BasicFilepath& filepath) { return os << filepath.path(); }

    size_t size() const { return length; }
};

struct FilepathHash {
    template <int N>
    size_t operator()(const BasicFilepath<N>& filepath) const { return std::hash<std::string_view>{}(filepath.path()); }
};

using Filepath = BasicFilepath<256>;
using LargeFilepath = BasicFilepath<512>;