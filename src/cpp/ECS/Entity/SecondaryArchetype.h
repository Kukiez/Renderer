#pragma once
#include <ECS/Entity/Entity.h>
#include <memory/type_info.h>
#include <memory/byte_arena.h>
#include <memory/free_list_allocator.h>
#include "ECS/Component/Types/PrimaryKindRegistry.h"

struct ArchetypeYieldType {
    Entity* entities;
    char* data;
    size_t size;
};

struct SecondaryEntityLocation {
    ByteBufferIndex bufferIndex = 0;
    void* data = nullptr;
    TypeUUID type;

    SecondaryEntityLocation() = default;
    SecondaryEntityLocation(const TypeUUID type) : type(type) {}
};

struct SecondaryEntityMetadata {
    mem::vector<SecondaryEntityLocation, mem::free_list_adaptor<SecondaryEntityLocation>> location;

    SecondaryEntityMetadata(mem::free_list_allocator* alloc) : location(alloc) {}

    SecondaryEntityLocation* find(const TypeUUID type) {
        const auto it = std::ranges::lower_bound(location, type, {}, &SecondaryEntityLocation::type);

        if (it != location.end() && it->type == type) {
            return &*it;
        }
        return nullptr;
    }

    void erase(SecondaryEntityLocation* loc) {
        auto it = location.begin() + (loc - location.data());
        location.erase(it);
    }

    SecondaryEntityLocation& findUnchecked(const TypeUUID type) {
        return *std::ranges::lower_bound(location, type, {}, &SecondaryEntityLocation::type);
    }

    void clear() {
        location.clear();
    }
};

class SecondaryArchetype {
    using Metadata = EntityMetadataStorage<SecondaryEntityMetadata>;
    using Arena = mem::byte_arena<mem::same_alloc_schema, 64>;

    mem::typeindex type = mem::type_info_of<void>;

    struct Chunk {
        char* buffer = nullptr;
        Entity* entities = nullptr;
        Arena allocator;
        size_t size = 0;
        size_t capacity = 0;
    };
    int expands = 0;
    std::vector<Chunk> chunks;
    ByteBufferIndex lastAvailableSpot = 0;
    TypeUUID typeID{};

    void initializeChunk(Chunk& chunk, size_t withCapacity) const;

    void expandChunk(Chunk& chunk, size_t withCapacity) const;

    std::tuple<ByteBufferIndex, void*, Entity*>   nextFree(size_t forEntities);

public:
    SecondaryArchetype() = default;

    SecondaryArchetype(const SecondaryArchetype&) = delete;
    SecondaryArchetype& operator = (const SecondaryArchetype&) = delete;

    SecondaryArchetype(SecondaryArchetype&& other) noexcept;

    SecondaryArchetype& operator = (SecondaryArchetype&& other) noexcept;

    ~SecondaryArchetype();

    static SecondaryArchetype create(TypeUUID typeID, mem::typeindex type);

    ByteBufferIndex reserve(size_t count);

    std::pair<ByteBufferIndex, void*> add(const Entity* entities, void** data, size_t count);

    void construct(const Entity& entity, void* data, ByteBufferIndex index);

    void remove(Metadata& metadata, const SecondaryEntityLocation& location);

    struct ArchetypeDataIterator {
        Chunk* first;

        using value_type = ArchetypeYieldType;
        
        bool operator == (const ArchetypeDataIterator other) const {
            return first == other.first;
        }

        bool operator != (const ArchetypeDataIterator other) const {
            return first != other.first;
        }

        ArchetypeYieldType operator*() const {
            return {
                first->entities, first->buffer, first->size
            };
        }

        ArchetypeDataIterator& operator ++ () {
            ++first;
            return *this;
        }

        ArchetypeDataIterator operator ++ (int) {
            auto temp = *this;
            ++first;
            return temp;
        }
    };

    auto begin() {
        return ArchetypeDataIterator{chunks.data()};
    }

    auto end() {
        return ArchetypeDataIterator{chunks.data() + chunks.size()};
    }

    void reset();
};