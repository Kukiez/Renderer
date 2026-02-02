#pragma once
#include <Renderer/Resource/Pass/RenderingPassKey.h>
#include <span>

class PassMask {
    std::bitset<64> mask{};
public:
    PassMask() = default;

    PassMask(std::initializer_list<RenderingPassType> passes) {
        for (auto pass : passes) {
            mask.set(pass.id());
        }
    }

    bool test(RenderingPassType pass) const {
        return mask.test(pass.id());
    }

    PassMask& operator |= (const PassMask& other) {
        mask |= other.mask;
        return *this;
    }

    void reset() {
        mask.reset();
    }

    template <IsRenderingPass... Ps>
    static PassMask of() {
        return {RenderingPassType::of<Ps>()...};
    }

    template <IsRenderingPass... Ps>
    PassMask& set() {
        (mask.set(RenderingPassType::of<Ps>().id()), ...);
        return *this;
    }
};

struct Primitive {
    AABB localBounds{};
    PassMask passes{};

    template <IsRenderingPass P>
    bool isInPass() const {
        return passes.test(RenderingPassType::of<P>().id());
    }
};

struct DefaultArray : IPrimitiveArray<Primitive> {};

class APrimitiveArray {
protected:
    friend class PrimitiveStorage;

    Primitive* basePrimitives{};
    size_t numPrimitives{};

    // T : public IPrimitiveArray<P>
    void* arrayData{};     // = T, NULL if DefaultArray
    void* primitiveData{}; // = P, NULL if DefaultArray

    AABB mergedLocalBounds{};
    PassMask mergedPasses{};

    std::atomic<unsigned> references = 0;

    mem::typeindex arrayType{};
    mem::typeindex primitiveType{};

    APrimitiveArray() = default;
    APrimitiveArray(Primitive* primitives,
        const size_t numPrimitives,
        void* arrayData,
        void* primitiveData)
    : basePrimitives(primitives),
    numPrimitives(numPrimitives),
    arrayData(arrayData),
    primitiveData(primitiveData) {}
public:
    APrimitiveArray(const APrimitiveArray&) = delete;
    APrimitiveArray& operator=(const APrimitiveArray&) = delete;
    APrimitiveArray(APrimitiveArray&&) = delete;
    APrimitiveArray& operator=(APrimitiveArray&&) = delete;

    const Primitive* data() const {
        return basePrimitives;
    }

    const void* getPrimitiveData() const {
        return primitiveData;
    }

    size_t size() const {
        return numPrimitives;
    }

    auto begin() {
        return basePrimitives;
    }

    auto end() {
        return basePrimitives + numPrimitives;
    }

    Primitive& operator [] (const size_t idx) {
        assert(idx < numPrimitives);
        return basePrimitives[idx];
    }

    const Primitive& operator [] (const size_t idx) const {
        assert(idx < numPrimitives);
        return basePrimitives[idx];
    }

    void recomputeLocalBounds() {
        AABB result;

        for (Primitive& primitive : std::span(basePrimitives, numPrimitives)) {
            result = geom::merge(result, primitive.localBounds);
        }
        mergedLocalBounds = result;
    }

    void recomputeLocalPasses() {
        mergedPasses.reset();

        for (Primitive& primitive : std::span(basePrimitives, numPrimitives)) {
            mergedPasses |= primitive.passes;
        }
    }

    const AABB& getLocalBounds() const { return mergedLocalBounds; }
    const PassMask& getPasses() const { return mergedPasses; }
};

template <typename P>
class PrimitiveReference;

template <typename P>
class PrimitiveReference<const P> {
    static constexpr bool HasPrimitiveData = !std::is_same_v<P, Primitive>;

    Primitive* primitive{};
    const P* primData{};
public:
    PrimitiveReference(Primitive* primitive, const P* primData) : primitive(primitive), primData(primData) {}

    const P* operator -> () requires HasPrimitiveData { return primData; }

    const P& operator * () requires HasPrimitiveData { return *primData; }

    const AABB& getLocalBounds() const { return primitive->localBounds; }
    const PassMask& getPasses() const { return primitive->passes; }
};

template <typename P>
class PrimitiveReference {
    static constexpr bool HasPrimitiveData = !std::is_same_v<P, Primitive>;

    Primitive* primitive{};
    P* primData{};
public:
    PrimitiveReference(Primitive* primitive, P* primData) : primitive(primitive), primData(primData) {}

    P* operator -> () requires HasPrimitiveData { return primData; }
    P& operator * () requires HasPrimitiveData { return *primData; }

    AABB& getLocalBounds() const { return primitive->localBounds; }
    PassMask& getPasses() const { return primitive->passes; }

    AABB& setLocalBounds(const AABB& bounds) const { return primitive->localBounds = bounds; }
    PassMask& setPasses(const PassMask& passes) const { return primitive->passes = passes; }

    PrimitiveReference& operator = (const Primitive& prim) {
        primitive->localBounds = prim.localBounds;
        primitive->passes = prim.passes;
        return *this;
    }

    PrimitiveReference& operator = (const P& pPrimData) requires HasPrimitiveData {
        *primData = *pPrimData;
        return *this;
    }
};


template <IsPrimitiveArray PArray>
class TPrimitiveArray : public APrimitiveArray {
    using PrimitiveType = typename PArray::PrimitiveType;

    static constexpr auto IsNotDefaultArray = !std::is_same_v<PrimitiveType, Primitive>;

    PrimitiveType* pdata() {
        return static_cast<PrimitiveType*>(primitiveData);
    }

    const PrimitiveType* pdata() const {
        return static_cast<const PrimitiveType*>(primitiveData);
    }
public:
    using APrimitiveArray::APrimitiveArray;

    PArray& operator * () requires IsNotDefaultArray {
        return *static_cast<PArray*>(arrayData);
    }

    PArray* operator -> () requires IsNotDefaultArray {
        return static_cast<PArray*>(arrayData);
    }

    const PArray& operator * () const requires IsNotDefaultArray {
        return *static_cast<const PArray*>(arrayData);
    }

    const PArray* operator -> () const requires IsNotDefaultArray {
        return static_cast<const PArray*>(arrayData);
    }

    PrimitiveReference<PrimitiveType> operator [] (const size_t idx) {
        assert(idx < numPrimitives);
        return PrimitiveReference<PrimitiveType>(basePrimitives + idx, pdata() + idx);
    }

    PrimitiveReference<const PrimitiveType> operator [] (const size_t idx) const {
        assert(idx < numPrimitives);
        return PrimitiveReference<const PrimitiveType>(basePrimitives + idx, pdata() + idx);
    }

    operator const TPrimitiveArray<const PArray>& () const requires (!std::is_const_v<PArray>)
    {
        return reinterpret_cast<const TPrimitiveArray<const PArray>&>(*this);
    }
};

using TDefaultPrimitiveArray = TPrimitiveArray<DefaultArray>;

template <IsPrimitiveArray PArray>
using TConstPrimitiveArray = const TPrimitiveArray<const PArray>&;