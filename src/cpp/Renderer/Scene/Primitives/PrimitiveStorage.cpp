#include "PrimitiveStorage.h"

PrimitiveStorage::~PrimitiveStorage() {
    for (auto& collection : collections) {
        if (collection.collectionData) {
            auto& type = allocators[collection.getType().id()].type;
            mem::destroy_at(type, collection.collectionData);
        }
    }

    for (auto& allocator : allocators) {
        for (auto& arena : allocator.allocators) {
            operator delete(arena.ptr, static_cast<std::align_val_t>(allocator.type.align()));
        }
    }
}

PrimitiveCollectionID PrimitiveStorage::createCollectionImpl(const Transform &transform, void *dataPtr,
    const APrimitiveArray& array, PrimitiveCollectionFlags flags, PrimitiveCollectionType type) {
    size_t idx = 0;

    auto& mArray = const_cast<APrimitiveArray&>(array);
    if (array.references == 0) {
        mArray.recomputeLocalBounds();
        mArray.recomputeLocalPasses();
    }
    ++mArray.references;

    PrimitiveCollectionID result;
    if (freeIndices.empty()) {
        idx = collections.size();
        collections.emplace_back(transform,
                                 dataPtr, type,
                                 array,
                                 flags
        );
        generations.emplace_back(0);

        result = PrimitiveCollectionID(0, static_cast<unsigned>(idx));
    } else {
        idx = freeIndices.back();
        freeIndices.pop_back();

        collections[idx] = PrimitiveCollection(transform,
                                               dataPtr, type,
                                               array,
                                               flags
        );

        result = PrimitiveCollectionID(generations[idx], static_cast<unsigned>(idx));
    }
    return result;
}

void * PrimitiveStorage::allocateMemoryForType(const PrimitiveCollectionType type, mem::typeindex memType) {
    if (allocators.size() <= type.id()) {
        allocators.resize(type.id() + 1);
    }

    auto& allocator = allocators[type.id()];

    if (void* mem = allocator.find()) {
        return mem;
    }
    allocator.type = memType;

    size_t nextCapacity = allocator.size() == 0 ? 4 : allocator.allocators[allocator.size() - 1].capacity * 2;

    allocator.allocators.emplace_back(
        static_cast<char *>(operator new(nextCapacity * memType.size(), static_cast<std::align_val_t>(memType.align()))),
        0, nextCapacity
    );
    return allocator.find();
}

void PrimitiveStorage::removeCollectionUnchecked(const PrimitiveCollectionID id) {
    if (id.gen() != generations[id.id()]) {
        return;
    }
    auto& collection = collections[id.id()];

    auto& array = const_cast<APrimitiveArray&>(collection.getPrimitivesArray());

    auto refCount = array.references.fetch_sub(1);

    if (refCount == 1) {

    }

    allocators[collection.getType().id()].free(collection.collectionData);

    collection.collectionData = nullptr;
    collection.collectionType = {};

    freeIndices.push_back(id.id());
    generations[id.id()]++;
}
