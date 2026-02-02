#pragma once
#include <iostream>
#include <limits>
#include <constexpr/Traits.h>
#include <glm/glm.hpp>
#include <memory/hash.h>
#include <memory/vector.h>

enum class ShaderBufferLayout {
    STD430,
    STD140,
    Unknown
};

enum class ShaderBuffer {
    Uniform,
    Storage,
    AtomicUInt,
    Unknown
};

enum class BufferBindingIndex : int {
    INVALID = std::numeric_limits<int>::max(),
};

enum class UniformParameterType {
    BOOL = 1,
    INT = 2,
    UINT = 3,
    FLOAT = 4,
    VEC2 = 5,
    VEC3 = 6,
    VEC4 = 7,
    IVEC2 = 8,
    IVEC3 = 9,
    IVEC4 = 10,
    UVEC2 = 11,
    UVEC3 = 12,
    UVEC4 = 13,
    MAT2 = 14,
    MAT3 = 15,
    MAT4 = 16,
    MAT2x3 = 17,
    MAT2x4 = 18,
    MAT3x2 = 19,
    MAT3x4 = 20,
    MAT4x2 = 21,
    MAT4x3 = 22,
    SAMPLER_1D = 23,
    SAMPLER_2D = 24,
    SAMPLER_3D = 25,
    SAMPLER_CUBE = 26,
    SAMPLER_2D_ARRAY = 27,
    SAMPLER_CUBE_ARRAY = 28,
    SAMPLER_BUFFER = 29,
    SAMPLER_2D_SHADOW = 30,
    UINT64,
    CLASS_TYPE
};

// Data Expected Layout: 0, 1, 2, 3, ..., full components
void deserialize(UniformParameterType type, std::string_view data, char* writePos, int arrayLen = 1);


inline std::ostream& operator<<(std::ostream& os, const ShaderBufferLayout& layout) {
    switch (layout) {
    case ShaderBufferLayout::STD430:
        os << "std430";
        break;
    case ShaderBufferLayout::STD140:
        os << "std140";
        break;
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const ShaderBuffer& buffer) {
    switch (buffer) {
    case ShaderBuffer::Uniform:
        os << "UniformBuffer";
        break;
    case ShaderBuffer::Storage:
        os << "StorageBuffer";
        break;
    case ShaderBuffer::AtomicUInt:
        os << "AtomicUIntBuffer";
        break;
    case ShaderBuffer::Unknown:
        os << "UnknownBuffer";
        break;
    }
    return os;
}

// hash: cexpr::type_hash(str)
UniformParameterType getUniformType(const uint64_t hash);

std::ostream& operator<<(std::ostream& os, UniformParameterType type);

struct ShaderString {
    std::string_view string;
    size_t hash = 0;

    ShaderString() = default;
    ShaderString(const char* mem, const size_t len, const size_t hash) : string(mem, len), hash(hash) {}

    static ShaderString fromString(const std::string_view str) {
        return {str.data(), str.length(), mem::string_hash{}(str)};
    }

    ShaderString(std::string_view str) : ShaderString(str.data(), str.length(), mem::string_hash{}(str)) {}

    template<size_t N>
    ShaderString(const char(&str)[N]) : ShaderString(str, N - 1, mem::string_hash{}(str)) {}

    friend std::ostream& operator<<(std::ostream& os, const ShaderString& param) {
        os << param.string;
        return os;
    }

    bool operator==(const ShaderString& other) const {
        return hash == other.hash && string == other.string;
    }

    bool operator!=(const ShaderString& other) const {
        return !(*this == other);
    }

    bool operator == (const std::string_view other) const {
        return string == other;
    }

    bool operator != (const std::string_view other) const {
        return !(*this == other);
    }

    operator std::string_view() const {
        return string;
    }

    size_t length() const {
        return string.length();
    }

    size_t size() const {
        return string.size();
    }

    std::string_view str() const {
        return string;
    }
};

template <> struct std::hash<ShaderString> {
    size_t operator()(const ShaderString& str) const noexcept {
        return str.hash;
    }
};

struct ShaderUniformParameter {
    ShaderString name{};
    UniformParameterType type;
    int location{};
    size_t offset = 0;

    friend std::ostream& operator<<(std::ostream& os, const ShaderUniformParameter& param) {
        os << param.name << ": " << param.type << "[" << param.location << "]";
        return os;
    }

    bool operator==(const ShaderUniformParameter& other) const {
        return name == other.name;
    }
};

size_t getUniformSizeAndAlign(UniformParameterType type, size_t& alignOut);

struct ShaderBufferParameter {
    ShaderString name;
    ShaderBuffer type = ShaderBuffer::Unknown;
    BufferBindingIndex binding = BufferBindingIndex::INVALID;
    ShaderBufferLayout layout = ShaderBufferLayout::STD430;

    friend std::ostream& operator<<(std::ostream& os, const ShaderBufferParameter& param) {
        os << param.name << ": " << param.type << " (" << param.layout << ", location: " << static_cast<int>(param.binding) << ")";
        return os;
    }

    bool operator==(const ShaderBufferParameter& other) const {
        return name == other.name;
    }

    bool operator!=(const ShaderBufferParameter& other) const {
        return !(*this == other);
    }
};

enum class UniformParameterIndex : unsigned {
    INVALID = std::numeric_limits<unsigned>::max(),
};

class ShaderUniformLayout {
    friend class ShaderDefinition;
    mem::vector<ShaderUniformParameter> myParameters{};
    size_t totalBytesRequired = 0;
public:
    ShaderUniformLayout() = default;
    explicit ShaderUniformLayout(mem::vector<ShaderUniformParameter>&& parameters);

    UniformParameterIndex getUniformParameter(const size_t nameHash) const;
    UniformParameterIndex getUniformParameter(const ShaderString& name) const;

    const ShaderUniformParameter& operator [] (const UniformParameterIndex index) const {
        return myParameters[static_cast<unsigned>(index)];
    }

    const auto& parameters() const {
        return myParameters;
    }

    auto& parameters() {
        return myParameters;
    }

    auto begin() const {
        return myParameters.begin();
    }

    auto end() const {
        return myParameters.end();
    }

    size_t size() const {
        return myParameters.size();
    }

    void calculateTotalBytesRequired();

    size_t totalBytesUsed() const {
        return totalBytesRequired;
    }
};