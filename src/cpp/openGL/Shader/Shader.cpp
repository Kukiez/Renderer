#include <charconv>

#include "ShaderReflection.h"

void deserialize(UniformParameterType type, std::string_view data, char *writePos, int arrayLen)
{
    auto parseFloat = [&](std::string_view s, float &out) {
        while (s.front() == ' ') s.remove_prefix(1);
        while (s.back() == ' ') s.remove_suffix(1);

        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
        return ec == std::errc{};
    };

    auto parseInt = [&](std::string_view s, int &out) {
        while (s.front() == ' ') s.remove_prefix(1);
        while (s.back() == ' ') s.remove_suffix(1);

        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
        return ec == std::errc{};
    };

    auto parseUint = [&](std::string_view s, unsigned &out) {
        while (s.front() == ' ') s.remove_prefix(1);
        while (s.back() == ' ') s.remove_suffix(1);

        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
        return ec == std::errc{};
    };

    auto split = [&](std::string_view sv, auto &&func) {
        size_t start = 0;
        for (;;) {
            size_t end = sv.find(',', start);
            std::string_view token = (end == std::string_view::npos)
                ? sv.substr(start)
                : sv.substr(start, end - start);

            func(token);

            if (end == std::string_view::npos)
                break;
            start = end + 1;
        }
    };

    switch (type) {
        case UniformParameterType::BOOL:
        case UniformParameterType::INT: {
            auto*& dst = reinterpret_cast<int*&>(writePos);
            parseInt(data, *dst);
            ++dst;
            break;
        }

        case UniformParameterType::UINT: {
            auto*& dst = reinterpret_cast<unsigned*&>(writePos);
            parseUint(data, *dst);
            ++dst;
            break;
        }

        case UniformParameterType::FLOAT: {
            auto*& dst = reinterpret_cast<float*&>(writePos);
            parseFloat(data, *dst);
            ++dst;
            break;
        }

        case UniformParameterType::VEC2:
        case UniformParameterType::VEC3:
        case UniformParameterType::VEC4: {
            auto*& dst = reinterpret_cast<float*&>(writePos);
            split(data, [&](std::string_view token) {
                parseFloat(token, *dst);
                dst++;
            });
            break;
        }

        case UniformParameterType::IVEC2:
        case UniformParameterType::IVEC3:
        case UniformParameterType::IVEC4: {
            auto*& dst = reinterpret_cast<int*&>(writePos);
            split(data, [&](std::string_view token) {
                parseInt(token, *dst);
                dst++;
            });
            break;
        }

        case UniformParameterType::UVEC2:
        case UniformParameterType::UVEC3:
        case UniformParameterType::UVEC4: {
            auto*& dst = reinterpret_cast<unsigned*&>(writePos);
            split(data, [&](std::string_view token) {
                parseUint(token, *dst);
                dst++;
            });
            break;
        }

        case UniformParameterType::MAT2:
        case UniformParameterType::MAT3:
        case UniformParameterType::MAT4:
        case UniformParameterType::MAT2x3:
        case UniformParameterType::MAT2x4:
        case UniformParameterType::MAT3x2:
        case UniformParameterType::MAT3x4:
        case UniformParameterType::MAT4x2:
        case UniformParameterType::MAT4x3: {
            auto *dst = reinterpret_cast<float*>(writePos);
            split(data, [&](std::string_view token) {
                parseFloat(token, *dst);
                dst++;
            });
            break;
        }

        default:
            throw std::runtime_error("Unsupported UniformParameterType in deserialize()");
    }
}

UniformParameterType getUniformType(const uint64_t hash) {
    switch (hash) {
        case cexpr::type_hash("float"): return UniformParameterType::FLOAT;
        case cexpr::type_hash("vec2"): return UniformParameterType::VEC2;
        case cexpr::type_hash("vec3"): return UniformParameterType::VEC3;
        case cexpr::type_hash("vec4"): return UniformParameterType::VEC4;
        case cexpr::type_hash("mat3"): return UniformParameterType::MAT3;
        case cexpr::type_hash("mat4"): return UniformParameterType::MAT4;
        case cexpr::type_hash("sampler2D"): return UniformParameterType::SAMPLER_2D;
        case cexpr::type_hash("sampler3D"): return UniformParameterType::SAMPLER_3D;
        case cexpr::type_hash("samplerCube"): return UniformParameterType::SAMPLER_CUBE;
        case cexpr::type_hash("sampler2DArray"): return UniformParameterType::SAMPLER_2D_ARRAY;
        case cexpr::type_hash("samplerCubeArray"): return UniformParameterType::SAMPLER_CUBE_ARRAY;
        case cexpr::type_hash("samplerBuffer"): return UniformParameterType::SAMPLER_BUFFER;
        case cexpr::type_hash("sampler2DShadow"): return UniformParameterType::SAMPLER_2D_SHADOW;
        case cexpr::type_hash("uint"): return UniformParameterType::UINT;
        case cexpr::type_hash("uvec2"): return UniformParameterType::UVEC2;
        case cexpr::type_hash("uvec3"): return UniformParameterType::UVEC3;
        case cexpr::type_hash("uvec4"): return UniformParameterType::UVEC4;
        case cexpr::type_hash("ivec2"): return UniformParameterType::IVEC2;
        case cexpr::type_hash("ivec3"): return UniformParameterType::IVEC3;
        case cexpr::type_hash("ivec4"): return UniformParameterType::IVEC4;
        case cexpr::type_hash("uint64_t"): return UniformParameterType::UINT64;
        case cexpr::type_hash("int"): return UniformParameterType::INT;
        case cexpr::type_hash("bool"): return UniformParameterType::BOOL;
        default: return UniformParameterType::CLASS_TYPE;
    }
}

std::ostream& operator<<(std::ostream& os, UniformParameterType type) {
    switch (type) {
        case UniformParameterType::BOOL:            os << "bool"; break;
        case UniformParameterType::INT:             os << "int"; break;
        case UniformParameterType::UINT:            os << "uint"; break;
        case UniformParameterType::FLOAT:           os << "float"; break;

        case UniformParameterType::VEC2:            os << "vec2"; break;
        case UniformParameterType::VEC3:            os << "vec3"; break;
        case UniformParameterType::VEC4:            os << "vec4"; break;

        case UniformParameterType::IVEC2:           os << "ivec2"; break;
        case UniformParameterType::IVEC3:           os << "ivec3"; break;
        case UniformParameterType::IVEC4:           os << "ivec4"; break;

        case UniformParameterType::UVEC2:           os << "uvec2"; break;
        case UniformParameterType::UVEC3:           os << "uvec3"; break;
        case UniformParameterType::UVEC4:           os << "uvec4"; break;

        case UniformParameterType::MAT2:            os << "mat2"; break;
        case UniformParameterType::MAT3:            os << "mat3"; break;
        case UniformParameterType::MAT4:            os << "mat4"; break;

        case UniformParameterType::MAT2x3:          os << "mat2x3"; break;
        case UniformParameterType::MAT2x4:          os << "mat2x4"; break;
        case UniformParameterType::MAT3x2:          os << "mat3x2"; break;
        case UniformParameterType::MAT3x4:          os << "mat3x4"; break;
        case UniformParameterType::MAT4x2:          os << "mat4x2"; break;
        case UniformParameterType::MAT4x3:          os << "mat4x3"; break;

        case UniformParameterType::SAMPLER_1D:          os << "sampler1D"; break;
        case UniformParameterType::SAMPLER_2D:          os << "sampler2D"; break;
        case UniformParameterType::SAMPLER_3D:          os << "sampler3D"; break;
        case UniformParameterType::SAMPLER_CUBE:        os << "samplerCube"; break;

        case UniformParameterType::SAMPLER_2D_ARRAY:    os << "sampler2DArray"; break;
        case UniformParameterType::SAMPLER_CUBE_ARRAY:  os << "samplerCubeArray"; break;

        case UniformParameterType::SAMPLER_BUFFER:      os << "samplerBuffer"; break;
        case UniformParameterType::SAMPLER_2D_SHADOW:   os << "sampler2DShadow"; break;

    default: os << "UNKNOWN"; break;
    }
    return os;
}

size_t getUniformSizeAndAlign(const UniformParameterType type, size_t &alignOut) {
    switch(type) {
        case UniformParameterType::BOOL:
        case UniformParameterType::INT:
        case UniformParameterType::UINT:
        case UniformParameterType::FLOAT:     alignOut = 4; return 4;

        case UniformParameterType::VEC2:
        case UniformParameterType::IVEC2:
            alignOut = 8; return 8;

        case UniformParameterType::VEC3:
        case UniformParameterType::IVEC3:
            alignOut = 16; return 12;

        case UniformParameterType::VEC4:      alignOut = 16; return 16;
        case UniformParameterType::MAT3:      alignOut = 16; return 48;
        case UniformParameterType::MAT4:      alignOut = 16; return 64;

        case UniformParameterType::SAMPLER_2D:
        case UniformParameterType::SAMPLER_3D:
        case UniformParameterType::SAMPLER_CUBE:
        case UniformParameterType::SAMPLER_2D_ARRAY:
        case UniformParameterType::SAMPLER_CUBE_ARRAY:
        case UniformParameterType::SAMPLER_BUFFER:   alignOut = 4; return 8;

        case UniformParameterType::UINT64: alignOut = 8; return 8;
        default:                              assert(false); return 0;
    }
}

ShaderUniformLayout::ShaderUniformLayout(mem::vector<ShaderUniformParameter> &&parameters): myParameters(std::move(parameters)) {
}

UniformParameterIndex ShaderUniformLayout::getUniformParameter(const size_t nameHash) const {
    for (size_t i = 0; i < myParameters.size(); ++i) {
        if (myParameters[i].name.hash == nameHash) {
            return UniformParameterIndex{static_cast<unsigned>(i)};
        }
    }
    return UniformParameterIndex::INVALID;
}

UniformParameterIndex ShaderUniformLayout::getUniformParameter(const ShaderString &name) const {
    for (size_t i = 0; i < myParameters.size(); ++i) {
        if (myParameters[i].name.hash == name.hash) {
            if (myParameters[i].name.string != name.string) continue;

            return UniformParameterIndex{static_cast<unsigned>(i)};
        }
    }
    return UniformParameterIndex::INVALID;
}

void ShaderUniformLayout::calculateTotalBytesRequired() {
    if (myParameters.empty()) return;

    size_t highestOffset = myParameters[0].offset;
    size_t index = 0;

    for (size_t i = 1; i < myParameters.size(); ++i) {
        if (myParameters[i].offset > highestOffset) {
            highestOffset = myParameters[i].offset;
            index = i;
        }
    }
    size_t alignOut;
    totalBytesRequired = highestOffset + getUniformSizeAndAlign(myParameters[index].type, alignOut);
}
