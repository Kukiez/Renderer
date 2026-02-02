#pragma once
#include <gl/glew.h>

enum class VertexAttributeType : uint8_t {
    BYTE,
    UNSIGNED_BYTE,
    SHORT,
    UNSIGNED_SHORT,
    INT,
    UNSIGNED_INT,
    FLOAT
};

constexpr GLenum opengl_enum_cast(const VertexAttributeType attr) {
    switch (attr) {
        case VertexAttributeType::BYTE: return GL_BYTE;
        case VertexAttributeType::UNSIGNED_BYTE: return GL_UNSIGNED_BYTE;
        case VertexAttributeType::SHORT: return GL_SHORT;
        case VertexAttributeType::UNSIGNED_SHORT: return GL_UNSIGNED_SHORT;
        case VertexAttributeType::INT: return GL_INT;
        case VertexAttributeType::UNSIGNED_INT: return GL_UNSIGNED_INT;
        case VertexAttributeType::FLOAT: return GL_FLOAT;
        default: std::unreachable();
    }
}


struct AttributeInfo {
    size_t components;
    size_t bytes;
    VertexAttributeType type;
};

template <typename T>
struct AttributeTraits;

namespace attrib {
    struct vec2 {
        float x, y;
    };
    struct vec3 {
        float x, y, z;
    };
    struct vec4 {
        float x, y, z, w;
    };
    struct ivec2 {
        int x, y;
    };
    struct ivec3 {
        int x, y, z;
    };
    struct ivec4 {
        int x, y, z, w;
    };
    struct uvec2 {
        unsigned int x, y;
    };
    struct uvec3 {
        unsigned int x, y, z;
    };
    struct uvec4 {
        unsigned int x, y, z, w;
    };
}

#define DEFINE_ATTRIBUTE_TRAIT(Class, ClassComposite, Components, Attr) template <> struct AttributeTraits<Class> { static constexpr AttributeInfo info{Components, sizeof(ClassComposite) * (Components), Attr}; };

DEFINE_ATTRIBUTE_TRAIT(attrib::vec2, float, 2, VertexAttributeType::FLOAT)
DEFINE_ATTRIBUTE_TRAIT(attrib::vec3, float, 3, VertexAttributeType::FLOAT)
DEFINE_ATTRIBUTE_TRAIT(attrib::vec4, float, 4, VertexAttributeType::FLOAT)
DEFINE_ATTRIBUTE_TRAIT(attrib::ivec2, int, 2, VertexAttributeType::INT)
DEFINE_ATTRIBUTE_TRAIT(attrib::ivec3, int, 3, VertexAttributeType::INT)
DEFINE_ATTRIBUTE_TRAIT(attrib::ivec4, int, 4, VertexAttributeType::INT)
DEFINE_ATTRIBUTE_TRAIT(attrib::uvec2, unsigned int, 2, VertexAttributeType::UNSIGNED_INT)
DEFINE_ATTRIBUTE_TRAIT(attrib::uvec3, unsigned int, 3, VertexAttributeType::UNSIGNED_INT)
DEFINE_ATTRIBUTE_TRAIT(attrib::uvec4, unsigned int, 4, VertexAttributeType::UNSIGNED_INT)
DEFINE_ATTRIBUTE_TRAIT(float, float, 1, VertexAttributeType::FLOAT)
DEFINE_ATTRIBUTE_TRAIT(int, int, 1, VertexAttributeType::INT)
DEFINE_ATTRIBUTE_TRAIT(unsigned int, unsigned int, 1, VertexAttributeType::UNSIGNED_INT)

template <typename Attr>
struct VertexAttributeInfo {
    static constexpr auto Info = AttributeTraits<Attr>::info;
    static constexpr size_t Components = AttributeTraits<Attr>::info.components;
    static constexpr VertexAttributeType Type = AttributeTraits<Attr>::info.type;
    static constexpr size_t Bytes = AttributeTraits<Attr>::info.bytes;
};

struct VertexAttribute {
    static constexpr auto Position3 = VertexAttributeInfo<attrib::vec3>{};
    static constexpr auto Normal3 = VertexAttributeInfo<attrib::vec3>{};
    static constexpr auto TexCoord2 = VertexAttributeInfo<attrib::vec2>{};
    static constexpr auto Tangent3 = VertexAttributeInfo<attrib::vec3>{};
    static constexpr auto Bitangent3 = VertexAttributeInfo<attrib::vec3>{};
    static constexpr auto Color4 = VertexAttributeInfo<attrib::vec4>{};
    static constexpr auto BoneIds4 = VertexAttributeInfo<attrib::ivec4>{};
    static constexpr auto BoneWeights4 = VertexAttributeInfo<attrib::vec4>{};

    static constexpr auto Position2 = VertexAttributeInfo<attrib::vec2>{};

    static constexpr auto FVEC2 = VertexAttributeInfo<attrib::vec2>{};
    static constexpr auto FVEC3 = VertexAttributeInfo<attrib::vec3>{};
    static constexpr auto FVEC4 = VertexAttributeInfo<attrib::vec4>{};

    static constexpr auto IVEC2 = VertexAttributeInfo<attrib::ivec2>{};
    static constexpr auto IVEC3 = VertexAttributeInfo<attrib::ivec3>{};
    static constexpr auto IVEC4 = VertexAttributeInfo<attrib::ivec4>{};

    static constexpr auto UVEC2 = VertexAttributeInfo<attrib::uvec2>{};
    static constexpr auto UVEC3 = VertexAttributeInfo<attrib::uvec3>{};
    static constexpr auto UVEC4 = VertexAttributeInfo<attrib::uvec4>{};
};

using VertexAttrib = VertexAttribute;