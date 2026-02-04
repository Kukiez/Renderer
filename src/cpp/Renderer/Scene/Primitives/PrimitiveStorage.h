#pragma once
#include <memory/free_list_allocator.h>

#include "Primitive.h"
#include "Renderer/Common/TypeAllocator.h"
#include "RendererAPI.h"

class RENDERERAPI PrimitiveStorage {
    std::vector<PrimitiveCollection> collections{};
    std::vector<TypeAllocatorArray> allocators{};

    std::vector<uint8_t> generations{};
    std::vector<unsigned> freeIndices{};

    mem::free_list_allocator primitivesAllocator;
public:
    explicit PrimitiveStorage(const size_t capacity) : primitivesAllocator(0.064 * 1024 * 1024) {
        collections.reserve(capacity);
    }

    PrimitiveStorage(const PrimitiveStorage&) = delete;
    PrimitiveStorage& operator=(const PrimitiveStorage&) = delete;
    PrimitiveStorage(PrimitiveStorage&&) = delete;
    PrimitiveStorage& operator=(PrimitiveStorage&&) = delete;

    ~PrimitiveStorage();

    PrimitiveCollectionID createCollectionImpl(const Transform &transform, void *dataPtr, const APrimitiveArray& array, PrimitiveCollectionFlags flags, PrimitiveCollectionType
                                               type);

    void* allocateMemoryForType(PrimitiveCollectionType type, mem::typeindex memType);

    template <IsBasePrimitiveCollection C>
    PrimitiveCollectionID createCollection(Transform transform, C&& data, const TDefaultPrimitiveArray& array, PrimitiveCollectionFlags flags) {
        using CType = std::decay_t<C>;

        auto dataType = PrimitiveCollectionType::of<CType>();
        void* dataPtr = allocateMemoryForType(dataType, mem::type_info::of<CType>());
        new (dataPtr) CType(std::forward<C>(data));

        return createCollectionImpl(transform, dataPtr, array, flags, dataType);
    }

    template <IsTypedPrimitiveCollection C>
    PrimitiveCollectionID createCollection(
        const Transform& transform,
        C&& data,
        const TPrimitiveArray<const typename std::decay_t<C>::PrimitiveArrayType>& array,
        PrimitiveCollectionFlags flags
    ) {
        using CType = std::decay_t<C>;

        auto dataType = PrimitiveCollectionType::of<CType>();
        void* dataPtr = allocateMemoryForType(dataType, mem::type_info::of<CType>());
        new (dataPtr) CType(std::forward<C>(data));

        return createCollectionImpl(transform, dataPtr, array, flags, dataType);
    }

    template <IsPrimitiveArray Array, typename... ArrayArgs>
    TPrimitiveArray<Array>* createPrimitiveArray(size_t numPrimitives, ArrayArgs&&... args) {
        auto* uPrimPtr = primitivesAllocator.allocate<typename Array::PrimitiveType>(numPrimitives);

        for (size_t i = 0; i < numPrimitives; ++i) {
            new (uPrimPtr + i) Array::PrimitiveType{};
        }

        Primitive* basePrimPtr = primitivesAllocator.allocate<Primitive>(numPrimitives);
        std::memset(basePrimPtr, 0, sizeof(Primitive) * numPrimitives);

        Array* arrayPtr = new (primitivesAllocator.allocate<Array>(1)) Array(std::forward<ArrayArgs>(args)...);

        return new (primitivesAllocator.allocate<TPrimitiveArray<Array>>(1)) TPrimitiveArray<Array>(basePrimPtr, numPrimitives, arrayPtr, uPrimPtr);
    }

    TDefaultPrimitiveArray* createPrimitiveArray(size_t numPrimitives) {
        Primitive* basePrimPtr = primitivesAllocator.allocate<Primitive>(numPrimitives);
        std::memset(basePrimPtr, 0, sizeof(Primitive) * numPrimitives);
        return new (primitivesAllocator.allocate<TDefaultPrimitiveArray>(1))  TDefaultPrimitiveArray(basePrimPtr, numPrimitives, nullptr, nullptr);
    }

    bool isCollectionValid(PrimitiveCollectionID id) const {
        if (id.gen() != generations[id.id()]) {
            return false;
        }
        return true;
    }

    PrimitiveCollection* getCollection(const PrimitiveCollectionID id) {
        if (!isCollectionValid(id)) {
            return nullptr;
        }
        return getCollectionUnchecked(id);
    }

    PrimitiveCollection* getCollectionUnchecked(const PrimitiveCollectionID id) {
        assert(id.id() < collections.size() && id.gen() == generations[id.id()]);
        return &collections[id.id()];
    }

    void removeCollectionUnchecked(PrimitiveCollectionID id);

    void removeCollection(const PrimitiveCollectionID id) {
        if (isCollectionValid(id)) {
            removeCollectionUnchecked(id);
        }
    }
};