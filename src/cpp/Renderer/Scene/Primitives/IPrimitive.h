#pragma once
#include <Renderer/Common/Transform.h>
#include <constexpr/Template.h>

struct DefaultArray;
struct Primitive;

struct IPrimitiveCollection {
    using PrimitiveArrayType = DefaultArray;
    using PrimitiveType = Primitive;
};

template <typename PrimType>
struct IPrimitiveArray {
    using PrimitiveType = PrimType;
};

template <typename T>
concept IsPrimitiveArray = cexpr::is_base_of_template<IPrimitiveArray, std::decay_t<T>>;

template <IsPrimitiveArray PrimArray>
struct ITPrimitiveCollection {
    using PrimitiveArrayType = PrimArray;
    using PrimitiveType = typename PrimArray::PrimitiveType;
};

template <typename T>
concept IsBasePrimitiveCollection = std::is_base_of_v<IPrimitiveCollection, std::decay_t<T>>;

template <typename T>
concept IsTypedPrimitiveCollection = cexpr::is_base_of_template<ITPrimitiveCollection, std::decay_t<T>>;

template <typename T>
concept IsPrimitiveCollection = IsBasePrimitiveCollection<T> || IsTypedPrimitiveCollection<T>;

template <typename T>
struct IPrimitiveWorld {};

template <typename T>
concept IsPrimitiveWorld = std::is_base_of_v<IPrimitiveWorld<T>, T>;

template <IsPrimitiveCollection C>
struct IPrimitivePipeline {
    using CollectionType = C;
};

struct IDynamicPrimitivePipeline {};

template <typename T>
concept IsDynamicPrimitivePipeline = std::is_base_of_v<IDynamicPrimitivePipeline, T>;

template <typename T>
concept IsStaticPrimitivePipeline = cexpr::is_base_of_template<IPrimitivePipeline, std::decay_t<T>>;

template <typename T>
concept IsPrimitivePipeline = IsDynamicPrimitivePipeline<T> || IsStaticPrimitivePipeline<T>;

enum class PrimitiveCollectionFlags : size_t {
    NONE = 0,
    INFINITE_BOUNDS = 1 << 0 // if set its position and bounds should be ignored
};

class PrimitiveCollectionChange {
    Transform oldTransform{};
    PrimitiveCollectionFlags oldFlags{};

    bool isTransformDirty = false;
    bool isFlagsDirty = false;
public:
    PrimitiveCollectionChange(Transform oldTransform, PrimitiveCollectionFlags oldFlags, bool isTransformDirty, bool isFlagsDirty)
    : oldTransform(oldTransform), oldFlags(oldFlags), isFlagsDirty(isFlagsDirty), isTransformDirty(isTransformDirty) {}

    bool isTransformChanged() const {
        return isTransformDirty;
    }

    bool isFlagsChanged() const {
        return isFlagsDirty;
    }

    const Transform& getOldTransform() const { return oldTransform; }
    PrimitiveCollectionFlags getOldFlags() const { return oldFlags; }
};

class PrimitiveCollectionID {
    unsigned myID : 24;
    unsigned myGen : 8;
public:
    constexpr PrimitiveCollectionID() : myID(0), myGen(0) {}

    constexpr PrimitiveCollectionID(unsigned gen, unsigned id) : myGen(gen), myID(id) {}

    unsigned id() const { return myID; }
    unsigned gen() const { return myGen; }

    bool operator ==(const PrimitiveCollectionID& other) const {
        return myID == other.myID && myGen == other.myGen;
    }

    bool operator !=(const PrimitiveCollectionID& other) const {
        return myID != other.myID || myGen != other.myGen;
    }
};