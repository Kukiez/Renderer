#pragma once
#include <memory/type_info.h>
#include <cassert>
#include <ECS/Entity/Entity.h>
#include <ECS/Component/ComponentRegistry.h>
#include <set>
#include <ECS/utils.h>
#include <algorithm>
#include <ECS/Entity/MetadataProvider.h>
#include <memory/byte_arena.h>
#include <memory/Span.h>
#include <ECS/Component/Types/PrimaryKindRegistry.h>

#include "memory/bitset.h"
#include "memory/PointerRange.h"

struct EntityTypeDataIterator;

struct EntityLocation {
    DataIndex dataIndex = std::numeric_limits<DataIndex>::max();
    ArchetypeIndex archIndex = std::numeric_limits<ArchetypeIndex>::max();
    ByteBufferIndex byteBuffer = std::numeric_limits<ByteBufferIndex>::max();

    EntityLocation() = default;

    template <typename T, typename U>
    EntityLocation(const T byteBufferIdx, const U dataIdx)
    : dataIndex(static_cast<DataIndex>(dataIdx)), byteBuffer(static_cast<ByteBufferIndex>(byteBufferIdx)) {}

    bool isValid() {
        return archIndex != std::numeric_limits<ArchetypeIndex>::max();
    }

    static EntityLocation invalid() {
        return {};
    }

    void clear() {
        *this = {};
    }
};

struct EntityMetadata {
    EntityLocation location;
};

class Archetype {
public:
    constexpr static DataIndex STARTING_CAPACITY = 32;
    constexpr static int MAX_STORAGES = 10;

    struct ArchetypeChunk {
        char* buffer = nullptr;

        char* data() const {
            return buffer;
        }
    };

    using Arena = mem::byte_arena<mem::same_alloc_schema, 64>;

    struct TypeIndex {
        ArchetypeChunk chunks[MAX_STORAGES]{};
        TypeUUID type{};
        const mem::type_info* typeInfo = mem::type_info_of<void>;
        mem::bitset<mem::byte_arena_adaptor<size_t, Arena>> changes[MAX_STORAGES]{};
        bool enableChanges = false;
        bool allBitsSet = false;

        TypeIndex() = default;
    };

    enum class NextFreeSpot {
        AVAILABLE,
        ARCHETYPE_FULL,
        UNINITIALIZED_STORAGE
    };

    struct InternalStorage {
        TypeIndex* typeIndices = nullptr;
        Entity* entities[MAX_STORAGES]{};
        Arena arenas[MAX_STORAGES]{};
        DataIndex sizes[MAX_STORAGES]{};
        DataIndex capacities[MAX_STORAGES]{};
        size_t maxTypes = 0;
        size_t types = 0;
        ByteBufferIndex lastIndex = 0;
        bool anyEnabledChanges = false;

        InternalStorage() = default;
        InternalStorage(InternalStorage&& other) noexcept;

        InternalStorage& operator = (InternalStorage&& other) = delete;

        NextFreeSpot nextFree(DataIndex forEntities, EntityLocation& location);

        PointerRange<TypeIndex> forEachType() {
            return {typeIndices, typeIndices + types};
        }
    } storage; 

    // allocates the types, once per archetype
    void allocateArchetype(size_t count);

    // allocates the shared arena
    Arena& allocateArena(ByteBufferIndex byteBufferIndex, DataIndex capacity);

    // allocates the first buffer, initializes type info
    // requires allocateArchetype() to be called before
    void allocateType(const ComponentField<PrimaryComponentField>* field, TypeUUID type);
    
    // allocation strategy for initializing first MAX_STORAGES allocators
    void allocateNewStorage(ByteBufferIndex byteBufferIdx, DataIndex capacity);

    // allocation strategy for when all allocators are initialized && previous is full
    void expandAllocator(ByteBufferIndex bufferIndex, DataIndex capacity);

    DataIndex getNextCapacityForIndex(ByteBufferIndex index);

    const EntityLocation& constructEntity(const Entity& e);

    EntityLocation constructEntityMulti(const Entity* entities, DataIndex count);

    void* getAt(const EntityLocation& loc, TypeUUID typeID);

    void* getAt(const EntityLocation& loc, TypeUUID typeID, size_t& hint);

    void* getAtTracked(const EntityLocation& loc, TypeUUID typeID, size_t& hint);
    void* getAtTracked(const EntityLocation& loc, TypeUUID typeID);

    void initializeEntity(const EntityLocation& location, EntityTypeDataIterator iterator) const;

    void eraseEntity(const Entity& e, const EntityLocation& loc);

    TypeIndex* findType(TypeUUID type);

    TypeIndex* findType(TypeUUID type, size_t hint);

    size_t expands = 0;
    DataIndex capacity = STARTING_CAPACITY;
    EntityMetadataStorage<EntityMetadata>* metadata = nullptr;

    Archetype() = default;

    Archetype(EntityMetadataStorage<EntityMetadata>* metadata) : metadata(metadata) {}

    Archetype(const Archetype& other) = delete;
    Archetype& operator = (const Archetype& other) = delete;

    Archetype(Archetype&& other) noexcept;

    Archetype& operator = (Archetype&& other) noexcept;

    ~Archetype();

    void overwriteEntity(const EntityLocation& loc, EntityTypeDataIterator iterator);

    void addEntity(const Entity& e, EntityTypeDataIterator iterator);

    void addEntities(const Entity* entities, DataIndex count, void** data);

    void deleteEntity(const Entity& e);
    
    void removeEntity(Archetype& dst, const Entity& e);

    void moveEntity(Archetype& dst, const Entity& e, EntityTypeDataIterator iterator);

    Entity** getEntities() {
        return storage.entities;
    }

    TypeIndex& findTypeIndex(TypeUUID type, size_t hint = 0) const;

    size_t indexOf(TypeIndex* typeIndex) const {
        if (typeIndex) {
            return typeIndex - storage.typeIndices;
        }
        return 0;
    }

    auto getTypeIterator() const {
        return mem::pointer_proj_iteratable<TypeIndex, [](auto& type) {return type.type; }>{
            storage.typeIndices,
            storage.typeIndices + storage.types
        };
    }

    size_t size(const int idx) const {
        return storage.sizes[idx];
    }

    auto* getTypeIndices() const {
        return storage.typeIndices;
    }

    DataIndex* getSizes() {
        return storage.sizes;
    }

    size_t getResidingEntitiesCount() const {
        size_t result = 0;

        for (const auto size : storage.sizes) {
            result += size;
        }
        return result;
    }

    void clearChanges() const;

    void reset();
};

template <IsTypeIterator... Iterators>
static Archetype createArchetypeFromTypeIterator(PrimaryKindRegistry& componentRegistry, EntityMetadataStorage<EntityMetadata>* metadata, Iterators&&... iterators) {
    Archetype arch(metadata);

    std::set<TypeUUID> types;

    ([&]{
        for (TypeUUID type : iterators) {
            types.insert(type);
        }        
    }(), ...);

    arch.allocateArchetype(types.size());

    for (auto& type : types) {
        arch.allocateType(componentRegistry.getFieldOf(type), type);
    }
    return arch;
}

template <typename... Ts>
static Archetype createArchetype(PrimaryKindRegistry& componentRegistry, EntityMetadataStorage<EntityMetadata>* metadata) {
    Archetype arch(metadata);

    arch.allocateArchetype(sizeof...(Ts));

    std::array<TypeUUID, sizeof...(Ts)> types = {
        componentRegistry.getTypeID<Ts>()...
    };
    std::ranges::sort(types);

    for (auto& type : types) {
        arch.allocateType(componentRegistry.getFieldOf(type), type);
    }
    return arch;
}

static Archetype createEmptyArchetype(EntityMetadataStorage<EntityMetadata>* metadata) {
    return {metadata};
}