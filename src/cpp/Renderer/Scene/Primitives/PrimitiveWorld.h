#pragma once
#include "PrimitiveStorage.h"
#include "IPrimitive.h"
#include "WorldCullCallback.h"

struct Frustum;

struct WorldVTable {
    using OnCullFn = void(*)(void*, const Frustum& frustum, WorldCullCallback callback);
    using OnAddCollectionFn = void(*)(void*, const PrimitiveCollectionID id, const PrimitiveCollection& collection);
    using OnRemoveCollectionFn = void(*)(void*, const PrimitiveCollectionID id, const PrimitiveCollection& collection);
    using OnUpdateCollectionFn = void(*)(void*, const PrimitiveCollectionID id, const PrimitiveCollection& collection, const PrimitiveCollectionChange& change);

    OnCullFn onCullFn{};
    OnAddCollectionFn onAddCollection{};
    OnRemoveCollectionFn onRemoveCollection{};
    OnUpdateCollectionFn onUpdateCollection{};

    template <typename W>
    static WorldVTable of() {
        WorldVTable vt{};

        vt.onCullFn = [](void* ptr, const Frustum& frustum, WorldCullCallback callback) {
            static_cast<W*>(ptr)->onCull(frustum, callback);
        };
        vt.onAddCollection = [](void* ptr, const PrimitiveCollectionID id, const PrimitiveCollection& collection) {
            static_cast<W*>(ptr)->onAddCollection(id, collection);
        };
        vt.onRemoveCollection = [](void* ptr, const PrimitiveCollectionID id, const PrimitiveCollection& collection) {
            static_cast<W*>(ptr)->onRemoveCollection(id, collection);
        };
        vt.onUpdateCollection = [](void* ptr, const PrimitiveCollectionID id, const PrimitiveCollection& collection, const PrimitiveCollectionChange &change) {
            static_cast<W*>(ptr)->onUpdateCollection(id, collection, change);
        };
        return vt;
    }
};

template <IsPrimitiveWorld T>
class TPrimitiveWorld;

class PrimitiveWorld {
protected:
    PrimitiveStorage* storage{};
    void* world{};
    WorldVTable worldVTable{};
public:
    PrimitiveWorld() = default;

    template <IsPrimitiveWorld W>
    PrimitiveWorld(PrimitiveStorage* storage, W* world) : storage(storage), world(world), worldVTable(WorldVTable::of<W>()) {}

    template <IsBasePrimitiveCollection C>
    PrimitiveCollectionID createCollection(const Transform& transform, C&& data, const TDefaultPrimitiveArray& array, PrimitiveCollectionFlags flags = PrimitiveCollectionFlags::NONE) {
        auto id = storage->createCollection(transform, std::forward<C>(data), array, flags);
        auto coll = storage->getCollectionUnchecked(id);

        worldVTable.onAddCollection(world, id, *coll);
        return id;
    }

    template <IsTypedPrimitiveCollection Collection>
    PrimitiveCollectionID createCollection(
        const Transform& transform,
        Collection&& collection,
        const TPrimitiveArray<const typename std::decay_t<Collection>::PrimitiveArrayType>& array,
        PrimitiveCollectionFlags flags = PrimitiveCollectionFlags::NONE
    ) {
        auto id = storage->createCollection(transform, std::forward<Collection>(collection), array, flags);
        auto coll = storage->getCollectionUnchecked(id);

        worldVTable.onAddCollection(world, id, *coll);
        return id;
    }

    void updateCollection(PrimitiveCollectionID collection, const Transform& newTransform, const PrimitiveCollectionFlags newFlags) {
        auto coll = storage->getCollection(collection);

        if (coll) {
            const PrimitiveCollectionChange change(coll->getWorldTransform(), coll->getFlags(), true, true);

            coll->worldTransform = newTransform;
            coll->flags = newFlags;

            coll->recomputeWorldBounds();

            worldVTable.onUpdateCollection(world, collection, *coll, change);
        }
    }

    void removeCollection(const PrimitiveCollectionID collection) {
        if (const auto coll = storage->getCollection(collection)) {
            worldVTable.onRemoveCollection(world, collection, *coll);
            storage->removeCollectionUnchecked(collection);
        }
    }

    const PrimitiveCollection* getCollection(const PrimitiveCollectionID id) const {
        return storage->getCollection(id);
    }

    template <IsPrimitiveCollection C>
    const TPrimitiveCollection<C>* getCollection(const PrimitiveCollectionID id) const {
        auto result = storage->getCollection(id);
        return result ? result->is<C>() : nullptr;
    }

    template <typename Callback>
    void cull(const Frustum& frustum, Callback&& callback) {
        using TCallback = std::decay_t<Callback>;
        WorldCullCallback cullCallback(&callback, [](const void* cb, PrimitiveCollectionID collection) {
            static_cast<const TCallback*>(cb)->operator()(collection);
        });
        worldVTable.onCullFn(world, frustum, cullCallback);
    }

    template <IsPrimitiveArray Array>
    TPrimitiveArray<Array>& createPrimitiveArray(const size_t numPrimitives) {
        return *storage->createPrimitiveArray<Array>(numPrimitives);
    }

    TDefaultPrimitiveArray& createPrimitiveArray(const size_t numPrimitives) {
        return *storage->createPrimitiveArray(numPrimitives);
    }


    PrimitiveStorage& getStorage() { return *storage; }
};

template <IsPrimitiveWorld T>
class TPrimitiveWorld : public PrimitiveWorld {
public:
    using PrimitiveWorld::PrimitiveWorld;

    T& operator * () {
        return *static_cast<T*>(world);
    }

    T* operator -> () {
        return static_cast<T*>(world);
    }

    T* getWorld() { return static_cast<T*>(world); }

    template <typename Callback>
    void cull(const Frustum& frustum, Callback&& callback) {
        WorldCullCallback cullCallback(&callback, [](const void* cb, PrimitiveCollectionID collection) {
            static_cast<const std::decay_t<Callback>*>(cb)->operator()(collection);
        });
        getWorld()->onCull(frustum, cullCallback);
    }
};