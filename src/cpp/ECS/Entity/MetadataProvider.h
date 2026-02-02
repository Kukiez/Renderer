#pragma once
#include <ECS/Entity/Entity.h>

template <typename Metadata>
class EntityMetadataStorage {
    Metadata* metadata = nullptr;
    size_t capacity = 0;
public:
    EntityMetadataStorage() = default;

    template <typename... InitArgs>
    EntityMetadataStorage(size_t initialCapacity, InitArgs&&... args)
     : capacity(initialCapacity), metadata((Metadata*)operator new(initialCapacity * sizeof(Metadata), std::align_val_t{alignof(Metadata)})) {
        for (auto i = 0ull; i < capacity; ++i) {
            new (metadata + i) Metadata(args...);
        }
    }

    EntityMetadataStorage(const EntityMetadataStorage&) = delete;
    EntityMetadataStorage(EntityMetadataStorage&&) = delete;

    ~EntityMetadataStorage() {
        operator delete(metadata, std::align_val_t{alignof(Metadata)});
    }

    Metadata* at(const Entity& entity) {
        return &metadata[entity.id()];
    }

    bool hasSpace(const EntityID entity) {
        if (entity < capacity) {
            return true;
        }
        return false;
    }

    Metadata& operator[](const Entity& entity) {
        return *at(entity);
    }

    template <typename... IArgs>
    void expand(size_t newCapacity, IArgs&&... iargs) {
        auto* newMetadata = static_cast<Metadata *>(operator new(sizeof(Metadata) * newCapacity, std::align_val_t{alignof(Metadata)}));

        for (auto i = 0ull; i < capacity; ++i) {
            new (newMetadata + i) Metadata(std::move(metadata[i]));
        }
        operator delete(metadata, std::align_val_t{alignof(Metadata)});
        metadata = newMetadata;

        for (auto i = capacity; i < newCapacity; ++i) {
            new (metadata + i) Metadata(iargs...);
        }
        capacity = newCapacity;
    }

    template <typename... Args>
    void reset(Args&&... args) {
        for (auto i = 0ull; i < capacity; ++i) {
            new (metadata + i) Metadata(args...);
        }
    }

    size_t getCapacity() const {
        return capacity;
    }
};