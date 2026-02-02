#pragma once
#include <bitset>
#include <Renderer/Common/Transform.h>
#include <Math/Shapes/geom.h>
#include "IPrimitive.h"
#include <util/enum_bit.h>
#include "PrimitiveArray.h"

class PrimitiveCollectionType : public ComponentIndex {
public:
    PrimitiveCollectionType() = default;
    PrimitiveCollectionType(ComponentIndex index) : ComponentIndex(index) {}

    template <IsPrimitiveCollection T>
    static PrimitiveCollectionType of() {
        return {ComponentIndexValue<PrimitiveCollectionType, std::decay_t<T>>::value};
    }
};

template <IsPrimitiveCollection C>
class TPrimitiveCollection;

class PrimitiveCollection {
protected:
    friend class PrimitiveWorld;
    friend class PrimitiveStorage;

    Transform worldTransform{};

    void* collectionData{};
    PrimitiveCollectionType collectionType{};

    const APrimitiveArray* primitives{};

    AABB worldBounds{};
    PrimitiveCollectionFlags flags{};
public:
    PrimitiveCollection(const Transform &wTransform, void* collData, PrimitiveCollectionType type, const APrimitiveArray &array, PrimitiveCollectionFlags flags)
    : worldTransform(wTransform), collectionData(collData), collectionType(type), primitives(&array), flags(flags) {
        recomputeWorldBounds();

        if ((size_t)&array <= 100) {
            assert(false);
        }
    }

    PrimitiveCollection(const PrimitiveCollection&) = delete;
    PrimitiveCollection& operator=(const PrimitiveCollection&) = delete;

    PrimitiveCollection(PrimitiveCollection&& other) noexcept = default;
    PrimitiveCollection& operator=(PrimitiveCollection&& other) noexcept = default;

    void* getInstance() {
        return collectionData;
    }

    const void* getInstance() const {
        return collectionData;
    }

    PrimitiveCollectionType getType() const {
        return collectionType;
    }

    AABB getLocalBounds() const {
        return primitives->getLocalBounds();
    }

    AABB getWorldBounds() const {
        return worldBounds;
    }

    void recomputeWorldBounds() {
        worldBounds = geom::transform(primitives->getLocalBounds(), worldTransform.createModel3D());
    }

    const Transform& getWorldTransform() const {
        return worldTransform;
    }

    std::span<const Primitive> getPrimitives() const {
        return primitives ? std::span(primitives->data(),primitives->size()) : std::span<const Primitive>();
    }

    template <IsPrimitiveCollection C>
    TPrimitiveCollection<C>* is() {
        if (collectionType == PrimitiveCollectionType::of<C>()) {
            return static_cast<TPrimitiveCollection<C>*>(this);
        }
        return nullptr;
    }

    template <IsPrimitiveCollection C>
    const TPrimitiveCollection<const C>* is() const {
        if (collectionType == PrimitiveCollectionType::of<C>()) {
            return static_cast<const TPrimitiveCollection<const C>*>(this);
        }
        return nullptr;
    }

    template <IsPrimitiveCollection C>
    TPrimitiveCollection<C>* cast() {
        return static_cast<TPrimitiveCollection<C>*>(this);
    }

    template <IsPrimitiveCollection C>
    const TPrimitiveCollection<const C>* cast() const {
        return static_cast<const TPrimitiveCollection<const C>*>(this);
    }

    auto begin() const {
        return getPrimitives().begin();
    }

    auto end() const {
        return getPrimitives().end();
    }

    PrimitiveCollectionFlags getFlags() const { return flags; }

    bool hasFlag(PrimitiveCollectionFlags flag) const { return (flags & flag) == flag; }

    const APrimitiveArray& getPrimitivesArray() const { return *primitives; }

    bool hasPrimitives() const {
        return primitives->size() > 0;
    }
};

template <IsPrimitiveCollection T>
class TPrimitiveCollection : public PrimitiveCollection {
    using PrimArrayType = typename T::PrimitiveArrayType;
    using PrimitiveType = typename PrimArrayType::PrimitiveType;
public:
    void* getInstance() = delete;

    T& operator * () {
        return *static_cast<T*>(PrimitiveCollection::getInstance());
    }

    T* operator -> () {
        return static_cast<T*>(PrimitiveCollection::getInstance());
    }

    const T& operator * () const {
        return *static_cast<const T*>(PrimitiveCollection::getInstance());
    }

    const T* operator -> () const {
        return static_cast<const T*>(PrimitiveCollection::getInstance());
    }

    const TPrimitiveArray<const PrimArrayType>& getArray() const {
        return *static_cast<const TPrimitiveArray<const PrimArrayType>*>(primitives);
    }

    PrimitiveReference<const PrimitiveType> operator [] (const size_t idx) const {
        return getArray()[idx];
    }
};