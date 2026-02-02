#pragma once
#include "Primitive.h"
#include "PrimitiveWorld.h"

using PrimitiveID = unsigned;

struct VisiblePrimitiveData {
    PrimitiveCollectionID collection{};
    PrimitiveID primitive{};
};

template <IsPrimitiveCollection C>
class VisiblePrimitive {
    PrimitiveWorld* world{};
    const VisiblePrimitiveData* data{};
public:
    VisiblePrimitive(PrimitiveWorld* world, const VisiblePrimitiveData* data) : world(world), data(data) {}

    const TPrimitiveCollection<const C>& getCollection() const {
        const PrimitiveCollection* coll = world->getStorage().getCollectionUnchecked(data->collection);
        return *coll->cast<C>();
    }

    const TPrimitiveArray<const typename C::PrimitiveArrayType>& getArray() const {
        return getCollection().getArray();
    }

    PrimitiveID getPrimitiveID() const { return data->primitive; }
    PrimitiveCollectionID getCollectionID() const { return data->collection; }

    PrimitiveReference<const typename C::PrimitiveType> getPrimitive() const {
        return getCollection()[data->primitive];
    }

    PrimitiveReference<const typename C::PrimitiveType> operator->() const {
        return getPrimitive();
    }

    PrimitiveReference<const typename C::PrimitiveType> operator*() const {
        return getPrimitive();
    }
};

template <IsPrimitiveCollection C>
class VisiblePrimitiveIterator {
    PrimitiveWorld* world{};
    const VisiblePrimitiveData* first{};
    const VisiblePrimitiveData* last{};
public:
    struct sentinel_end {};

    VisiblePrimitiveIterator(PrimitiveWorld* world, const VisiblePrimitiveData* first, const VisiblePrimitiveData* last) : world(world), first(first), last(last) {}

    VisiblePrimitiveIterator& operator++() {
        ++first;
        return *this;
    }

    VisiblePrimitiveIterator operator ++ (int) {
        VisiblePrimitiveIterator copy = *this;
        ++first;
        return copy;
    }

    bool operator==(const sentinel_end&) const {
        return first == last;
    }

    bool operator!=(const sentinel_end) const {
        return first != last;
    }

    VisiblePrimitive<C> operator*() {
        return VisiblePrimitive<C>(world, first);
    }
};

template <IsPrimitiveCollection C>
class VisiblePrimitiveIteratable {
    PrimitiveWorld* world{};
    const VisiblePrimitiveData* data{};
    const VisiblePrimitiveData* last{};
public:
    VisiblePrimitiveIteratable(PrimitiveWorld* world, const VisiblePrimitiveData* data, const VisiblePrimitiveData* last) : world(world), data(data), last(last) {}

    auto begin() {
        return VisiblePrimitiveIterator<C>(world, data, last);
    }

    constexpr static auto end() {
        return typename VisiblePrimitiveIterator<C>::sentinel_end{};
    }

    size_t size() const {
        return last - data;
    }

    bool empty() const {
        return data == last;
    }

    VisiblePrimitive<C> operator[](size_t index) {
        assert(index < size());
        return VisiblePrimitive<C>(world, data + index);
    }
};

class VisiblePrimitiveList {
    Renderer* renderer{};
    PrimitiveWorld* world{};

    std::vector<std::vector<VisiblePrimitiveData>> data{};
public:
    VisiblePrimitiveList() = default;
    VisiblePrimitiveList(Renderer* renderer, PrimitiveWorld* world) : renderer(renderer), world(world) {}

    void draw(const PrimitiveCollectionType type, const PrimitiveCollectionID collection, const PrimitiveID primitive) {
        if (data.size() <= type.id()) {
            data.resize(type.id() + 1);
        }

        data[type.id()].emplace_back(collection, primitive);
    }

    template <IsPrimitiveCollection C>
    VisiblePrimitiveIteratable<C> getVisible() const {
        auto id = PrimitiveCollectionType::of<C>();

        if (data.size() <= id.id()) {
            return VisiblePrimitiveIteratable<C>(world, nullptr, nullptr);
        }
        return VisiblePrimitiveIteratable<C>(world, data[id.id()].data(), data[id.id()].data() + data[id.id()].size());
    }

    size_t getUniqueCollectionTypes() const {
        return data.size();
    }
};