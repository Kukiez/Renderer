#pragma once
#include "ShaderReflection.h"

class ShaderCache;
class ShaderClass;

struct ShaderClassMember {
    struct Type {
        union {
            const ShaderClass* classType{};
            UniformParameterType primitiveType;
        };
        bool isClassType{};

        Type(const ShaderClass* type) : classType(type), isClassType(true) {}
        Type(const UniformParameterType type) : primitiveType(type), isClassType(false) {}

        bool operator==(const Type & other) const;

        size_t size() const;

        size_t alignment() const;

        friend std::ostream& operator<<(std::ostream& os, const Type& type);
    };

    ShaderString name{};
    Type type;
    size_t offset = 0;
    size_t arrayLen = 1;

    friend std::ostream& operator<<(std::ostream& os, const ShaderClassMember& param);

    bool operator==(const ShaderClassMember& other) const {
        return name == other.name
            && type == other.type
            && arrayLen == other.arrayLen;
    }

    bool isEqualLayout(const ShaderClassMember& other) const {
        return type == other.type && arrayLen == other.arrayLen;
    }

    size_t size() const {
        return type.size() * arrayLen;
    }

    size_t alignment() const {
        return type.alignment();
    }

    bool isArray() const {
        return arrayLen > 1;
    }

    bool isClassType() const {
        return type.isClassType;
    }

    bool isPrimitiveType() const {
        return !type.isClassType;
    }

    size_t arrayLength() const {
        return arrayLen;
    }
};

enum class ShaderClassMemberIndex : unsigned {
    INVALID = std::numeric_limits<unsigned>::max()
};

enum class ShaderClassPackingMode {
    TIGHT,
    STD_140,
    STD_430,
    CUSTOM,
    DEFAULT = STD_430
};

using ShaderClassPackingModeFn = void(*)(std::vector<ShaderClassMember>& members, size_t& size, size_t& alignment);

struct ShaderClassStd430PackingMode {
    static void pack(std::vector<ShaderClassMember>& members, size_t& size, size_t& alignment);
};

class ShaderClass {
    friend class ShaderClassBuilder;

    ShaderString myName{};
    std::vector<ShaderClassMember> myMembers{};
    size_t myAlignment{};
    size_t mySize{};
public:
    ShaderClass() = default;

    ShaderClass(const ShaderClass& other) = delete;
    ShaderClass& operator=(const ShaderClass& other) = delete;

    ShaderClass(ShaderClass&& other) noexcept = default;
    ShaderClass& operator=(ShaderClass&& other) noexcept = default;

    size_t alignment() const { return myAlignment; }
    size_t size() const { return mySize; }

    ShaderString name() const { return myName; }

    bool isNull() const { return myName.hash == 0; }

    bool isEqualLayout(const ShaderClass& other) const;

    bool isEqual(const ShaderClass& other) const;

    static const ShaderClass& fromString(ShaderCache& cache, std::string_view classDef,
        ShaderClassPackingMode packingMode = ShaderClassPackingMode::DEFAULT, ShaderClassPackingModeFn packingModeFn = nullptr);

    static void pack(ShaderClassPackingMode packingMode, std::vector<ShaderClassMember>& members,
        size_t& size, size_t& alignment, ShaderClassPackingModeFn packingModeFn = nullptr);

    friend std::ostream& operator<<(std::ostream& os, const ShaderClass& shaderClass);

    auto members() const { return mem::make_range(myMembers.data(), myMembers.size()); }

    ShaderClass clone() const;

    const ShaderClassMember* findMember(const ShaderString& member) const;

    ShaderClassMemberIndex findMemberIndex(const ShaderString& member) const;

    const ShaderClassMember& operator[](ShaderClassMemberIndex index) const { return myMembers[static_cast<size_t>(index)]; }

    void printAsInstance(std::ostream& os, const void* data) const;
};

class ShaderClassBuilder {
    ShaderCache& cache;

    ShaderClass myClass{};

    ShaderClassPackingMode packingMode = ShaderClassPackingMode::DEFAULT;
    ShaderClassPackingModeFn packingModeFn{};
public:
    explicit ShaderClassBuilder(ShaderCache& cache, size_t reserveMembers = 2);

    void reserve(size_t members);

    void setName(std::string_view name);

    void setPackingMode(ShaderClassPackingMode packingMode, ShaderClassPackingModeFn packingModeFn = nullptr);

    void setAlignment(size_t alignment);

    void addMember(std::string_view name, UniformParameterType type, size_t arrayLen = 0);
    void addMember(std::string_view name, const ShaderClass& type, size_t arrayLen = 0);
    void addMember(std::string_view name, const ShaderClassMember& member);

    void addMembers(const ShaderClass& other);

    const ShaderClass& build();
};