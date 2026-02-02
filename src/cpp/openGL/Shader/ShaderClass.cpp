#include "ShaderClass.h"

#include <ranges>
#include <regex>
#include <Renderer/Resource/Shader/Types/ValueConstructor.h>

#include "ShaderCache.h"

bool ShaderClassMember::Type::operator==(const Type &other) const {
    if (isClassType && !other.isClassType) return false;

    if (isClassType) {
        return classType == other.classType;
    }
    return primitiveType == other.primitiveType;
}

size_t ShaderClassMember::Type::size() const {
    if (isClassType) {
        return classType->size();
    }
    size_t align = 0;
    return getUniformSizeAndAlign(primitiveType, align);
}

size_t ShaderClassMember::Type::alignment() const {
    if (isClassType) {
        return classType->alignment();
    }
    size_t align = 0;
    getUniformSizeAndAlign(primitiveType, align);
    return align;
}

void ShaderClassStd430PackingMode::pack(std::vector<ShaderClassMember> &members, size_t &size, size_t &alignment) {
    size_t currentOffset = 0;
    size_t highestAlignment = 0;

    if (members.empty()) return;

    for (auto& member : members) {
        const size_t align = member.alignment();
        const size_t alignedOffset = (currentOffset + align - 1) / align * align;

        member.offset = alignedOffset;

        currentOffset = alignedOffset + member.size();

        highestAlignment = std::max(highestAlignment, align);
    }

    alignment = highestAlignment;

    const size_t alignedSize = (currentOffset + alignment - 1) / alignment * alignment;

    size = alignedSize;
}

const ShaderClass& ShaderClass::fromString(ShaderCache &cache, std::string_view classDef, ShaderClassPackingMode packingMode,
    ShaderClassPackingModeFn packingModeFn) {
    const auto structKeyword = classDef.find("struct ");

    if (structKeyword == std::string_view::npos) {
        return cache.getNullShaderClass();
    }

    const auto nameStart = structKeyword + 7;

    size_t nameEnd = nameStart;

    while (nameEnd < classDef.size()) {
        if (classDef[nameEnd] == '{' || classDef[nameEnd] == ' ') {
            break;
        }
        ++nameEnd;
    }

    if (nameEnd == classDef.size()) {
        return cache.getNullShaderClass();
    }

    auto className = classDef.substr(nameStart, nameEnd - nameStart);

    const auto bracketBegin = classDef.find('{', nameEnd);

    if (bracketBegin == std::string_view::npos) {
        return cache.getNullShaderClass();
    }

    const auto bracketEnd = classDef.find_first_of('}', bracketBegin);

    if (bracketEnd == std::string_view::npos) {
        return cache.getNullShaderClass();
    } // TODO ERRORS

    ShaderClassBuilder builder(cache, 8);
    builder.setName(className);
    builder.setPackingMode(packingMode, packingModeFn);

    std::string_view memberDef(classDef.begin() + bracketBegin + 1, classDef.begin() + bracketEnd);

    static std::regex regex(R"(\s+(\w+)\s+(\w+)\s*(\[\s*\d+\s*\])?\s*;)");

    std::string part(memberDef.begin(), memberDef.end()); // TODO fuck std::regex

    auto begin = std::sregex_iterator(part.begin(), part.end(), regex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        auto& match = *it;
        std::string_view type(match[1].first._Ptr, match[1].length());
        std::string_view name(match[2].first._Ptr, match[2].length());
        std::string_view arrayLen(match[3].first._Ptr, match[3].length());

        size_t uTypeHash = cexpr::type_hash(type.data(), type.length());

        auto typeEnum = getUniformType(uTypeHash);

        size_t arrayLenInt = 1;

        if (!arrayLen.empty()) {
            std::string arrayLenStr(arrayLen.data() + 1, arrayLen.length() - 2);

            arrayLenInt = std::stoi(arrayLenStr);
        }

        if (typeEnum == UniformParameterType::CLASS_TYPE) {
            auto& classType = cache.findShaderClass(cache.internString(type));

            if (classType.isNull()) {
                return cache.getNullShaderClass();
            }

            builder.addMember(name, classType, arrayLenInt);
        } else {
            builder.addMember(name, typeEnum, arrayLenInt);
        }
    }

    return builder.build();
}

bool ShaderClass::isEqualLayout(const ShaderClass &other) const {
    if (myAlignment != other.myAlignment) {
        return false;
    }
    if (mySize != other.mySize) {
        return false;
    }
    return std::ranges::equal(myMembers, other.myMembers, [](const ShaderClassMember& a, const ShaderClassMember& b) {
        return a.isEqualLayout(b);
    });
}

bool ShaderClass::isEqual(const ShaderClass &other) const {
    if (myName != other.myName) {
        return false;
    }
    if (mySize != other.mySize) {
        return false;
    }
    if (myAlignment != other.myAlignment) {
        return false;
    }
    return std::ranges::equal(myMembers, other.myMembers);
}

void ShaderClass::pack(ShaderClassPackingMode packingMode, std::vector<ShaderClassMember> &members, size_t &size,
    size_t &alignment, ShaderClassPackingModeFn packingModeFn)
{
    switch (packingMode) {
        case ShaderClassPackingMode::STD_140: {
            assert(false);
        }
        case ShaderClassPackingMode::STD_430: {
            ShaderClassStd430PackingMode::pack(members, size, alignment);
            break;
        }
        case ShaderClassPackingMode::CUSTOM: {
            if (packingModeFn) {
                packingModeFn(members, size, alignment);
            }
            break;
        }
        default: assert(false); // TODO make
    }
}

ShaderClass ShaderClass::clone() const {
    ShaderClass other;
    other.myName = myName;
    other.myMembers.reserve(myMembers.size());
    std::ranges::copy(myMembers, std::back_inserter(other.myMembers));
    other.myAlignment = myAlignment;
    other.mySize = mySize;
    return other;
}

const ShaderClassMember * ShaderClass::findMember(const ShaderString &member) const {
    for (const auto& m : myMembers) {
        if (m.name == member) {
            return &m;
        }
    }
    return nullptr;
}

ShaderClassMemberIndex ShaderClass::findMemberIndex(const ShaderString &member) const {
    for (size_t i = 0; i < myMembers.size(); ++i) {
        if (myMembers[i].name == member) {
            return static_cast<ShaderClassMemberIndex>(i);
        }
    }
    return ShaderClassMemberIndex::INVALID;
}

void ShaderClass::printAsInstance(std::ostream &os, const void *data) const {
    auto chData = static_cast<const char*>(data);

    os << "Instance of " << name() << ":\n";
    for (const auto& member : myMembers) {
        os << '\t' << member.name << ": ";

        if (member.isPrimitiveType()) {
            ShaderValueConstructor value(member.type.primitiveType, chData + member.offset);
            os << value;
        } else {
        }

        os << ",\n";
    }
    std::cout << std::flush;
}

ShaderClassBuilder::ShaderClassBuilder(ShaderCache &cache, const size_t reserveMembers): cache(cache) {
    myClass.myMembers.reserve(reserveMembers);
}

void ShaderClassBuilder::reserve(size_t members) {
    myClass.myMembers.reserve(members);
}

void ShaderClassBuilder::setName(std::string_view name) {
    myClass.myName = cache.internString(name);
}

void ShaderClassBuilder::setPackingMode(ShaderClassPackingMode packingMode, ShaderClassPackingModeFn packingModeFn) {
    this->packingMode = packingMode;
    this->packingModeFn = packingModeFn;
}

void ShaderClassBuilder::setAlignment(size_t alignment) {
    myClass.myAlignment = alignment;
}

void ShaderClassBuilder::addMember(std::string_view name, UniformParameterType type, size_t arrayLen) {
    myClass.myMembers.emplace_back(cache.internString(name), type, 0, arrayLen);
}

void ShaderClassBuilder::addMember(std::string_view name, const ShaderClass &type, size_t arrayLen) {
    myClass.myMembers.emplace_back(cache.internString(name), &type, 0, arrayLen);
}

void ShaderClassBuilder::addMember(std::string_view name, const ShaderClassMember &member) {
    myClass.myMembers.emplace_back(cache.internString(name), member.type, 0, member.arrayLen);
}

void ShaderClassBuilder::addMembers(const ShaderClass &other) {
    myClass.myMembers.insert(myClass.myMembers.end(), other.myMembers.begin(), other.myMembers.end());
}

const ShaderClass & ShaderClassBuilder::build() {
    switch (packingMode) {
        case ShaderClassPackingMode::STD_140: {
            assert(false);
        }
        case ShaderClassPackingMode::STD_430: {
            ShaderClassStd430PackingMode::pack(myClass.myMembers, myClass.mySize, myClass.myAlignment);
            break;
        }
        default: assert(false); // TODO make
    }

    return cache.addShaderClass(std::move(myClass));
}

std::ostream & operator<<(std::ostream &os, const ShaderClassMember::Type &type) {
    if (type.isClassType) {
        os << type.classType->name();
    } else {
        os << type.primitiveType;
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const ShaderClassMember &param) {
    os << param.name << ": " << param.type;

    if (param.isArray()) {
        os << "[" << param.arrayLen << "]";
    }
    os << " (+" << param.offset << ")";
    return os;
}

std::ostream & operator<<(std::ostream &os, const ShaderClass &shaderClass) {
    if (shaderClass.isNull()) {
        return os << "Class: Null";
    }

    os << "Class: " << shaderClass.name() << "[Size: " << shaderClass.size() << ", Align: " << shaderClass.alignment() << "]\n";
    for (const auto& member : shaderClass.myMembers) {
        os << "  " << member << "\n";
    }
    return os;
}
