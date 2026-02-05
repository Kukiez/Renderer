#include "Archetype.h"
#include "ComponentStorage2.h"
#include "BooleanStorage.h"
#include "EntityRegistry.h"
#include <ECS/Component/Types/BooleanComponentType.h>

#include "SecondaryArchetype.h"
#include "SparseComponentStorage.h"

Archetype::InternalStorage::InternalStorage(InternalStorage&& other) noexcept
: typeIndices(other.typeIndices), maxTypes(other.maxTypes), types(other.types), lastIndex(other.lastIndex), anyEnabledChanges(other.anyEnabledChanges) {
    memcpy(entities, other.entities, sizeof(entities));
    memcpy(sizes, other.sizes, sizeof(sizes));
    memcpy(capacities, other.capacities, sizeof(capacities));

    memset(sizes, 0, sizeof(sizes));
    memset(capacities, 0, sizeof(capacities));
    memset(entities, 0, sizeof(entities));

    for (int i = 0; i < MAX_STORAGES; ++i) {
        arenas[i] = std::move(other.arenas[i]);
    }
    other.typeIndices = nullptr;
    other.maxTypes = 0;
    other.types = 0;
    other.lastIndex = 0;
    other.anyEnabledChanges = false;
}

Archetype::NextFreeSpot Archetype::InternalStorage::nextFree(const DataIndex forEntities, EntityLocation& location) {
    for (auto i = 0; i < MAX_STORAGES; ++i) {
        const auto capacity = capacities[i];
        const auto size = sizes[i];

        if (capacity == 0) {
            location.byteBuffer = i;
            location.dataIndex = 0;
            return NextFreeSpot::UNINITIALIZED_STORAGE;
        }
        if (size + forEntities <= capacity) {
            location.byteBuffer = i;
            location.dataIndex = size;
            return NextFreeSpot::AVAILABLE;
        }
    }
    return NextFreeSpot::ARCHETYPE_FULL;
}

void Archetype::allocateArchetype(const size_t count) {
    storage.typeIndices = new TypeIndex[count];
    storage.maxTypes = count;
}

Archetype::Arena& Archetype::allocateArena(ByteBufferIndex byteBufferIndex, DataIndex capacity) {
    mem::bytes_required req;
    req.include(mem::type_info::of<Entity>(), capacity);

    for (auto& type : storage.forEachType()) {
        req.include(type.typeInfo, capacity);

        if (type.enableChanges) {
            req.include(mem::type_info_of<size_t>, capacity / sizeof(size_t) + (capacity % sizeof(size_t) != 0));
        }
    }
    storage.arenas[byteBufferIndex].initialize(req);
    storage.capacities[byteBufferIndex] = capacity;
    return storage.arenas[byteBufferIndex];
}

void Archetype::allocateType(const ComponentField<PrimaryComponentField>* field, const TypeUUID type) {
    TypeIndex& typeIndex = storage.typeIndices[storage.types];
    typeIndex.type = type;
    typeIndex.typeInfo = field->type;
    typeIndex.enableChanges = field->isTrackedComponent;
    storage.anyEnabledChanges |= field->isTrackedComponent;
    ++storage.types;
}

void Archetype::allocateNewStorage(const ByteBufferIndex byteBufferIdx, const DataIndex capacity) {
    auto& arena = allocateArena(byteBufferIdx, capacity);
    storage.entities[byteBufferIdx] = arena.allocate<Entity>(capacity);
    for (int i = 0; i < storage.types; ++i) {
        TypeIndex& typeIndex = storage.typeIndices[i];
        void* ptr = arena.allocate(typeIndex.typeInfo, capacity);
        typeIndex.chunks[byteBufferIdx].buffer = static_cast<char*>(ptr);

        if (typeIndex.enableChanges) {
            typeIndex.changes[byteBufferIdx] = mem::make_bitset(
                mem::make_byte_arena_adaptor<size_t, Arena>(arena),
                capacity
            );
        }
    }
    ++expands;
}

void Archetype::expandAllocator(const ByteBufferIndex bufferIndex, const DataIndex capacity) {
    // temporarily preserve old data to move
    auto prevArena = std::move(storage.arenas[bufferIndex]);
    auto& arena = allocateArena(bufferIndex, capacity);
    auto* newEntities = arena.allocate<Entity>(capacity);

    const auto size = storage.sizes[bufferIndex];
    for (int i = 0; i < storage.types; ++i) {
        TypeIndex& typeIndex = storage.typeIndices[i];
        const auto newPtr = static_cast<char *>(arena.allocate(typeIndex.typeInfo, capacity));

        char*& oldPtr = typeIndex.chunks[bufferIndex].buffer;

        const auto type = typeIndex.typeInfo;
        type.move(newPtr, oldPtr, size);
        type.destroy(oldPtr, size);

        oldPtr = newPtr;
    }

    if (storage.anyEnabledChanges) {
        for (int i = 0; i < storage.types; ++i) {
            TypeIndex& typeIndex = storage.typeIndices[i];

            if (typeIndex.enableChanges) {
                auto& bitset = typeIndex.changes[bufferIndex];
                size_t* oldBits = bitset.data();
                bitset = mem::make_bitset(
                    mem::make_byte_arena_adaptor<size_t>(arena),
                    capacity
                );
                mem::memcpy(bitset.data(), oldBits, size * sizeof(size_t));
            }
        }
    }
    Entity*& oldEntities = storage.entities[bufferIndex];

    memcpy(newEntities, oldEntities, sizeof(Entity) * size);
    oldEntities = newEntities;
}

DataIndex Archetype::getNextCapacityForIndex(const ByteBufferIndex index) {
    capacity *= 2;
    return capacity;
}

const EntityLocation& Archetype::constructEntity(const Entity& e) {
    auto& loc = metadata->at(e)->location;

    switch (storage.nextFree(1, loc)) {
        case NextFreeSpot::UNINITIALIZED_STORAGE:
            capacity *= 2;
            allocateNewStorage(loc.byteBuffer, capacity);
            break;
        case NextFreeSpot::ARCHETYPE_FULL: {
            capacity *= 2;
            const ByteBufferIndex bufferIndex = expands % MAX_STORAGES;
            expandAllocator(bufferIndex, capacity);
            loc.byteBuffer = bufferIndex;
            loc.dataIndex = storage.sizes[bufferIndex];
            ++expands;
            break;
        }
        default: break;
    }
    return loc;
}

EntityLocation Archetype::constructEntityMulti(const Entity* entities, const DataIndex count) {
    EntityLocation firstLocation;

    switch (storage.nextFree(count, firstLocation)) {
        case NextFreeSpot::UNINITIALIZED_STORAGE:
            capacity = std::max(capacity * 2, mem::round_up_to_64(count));
            allocateNewStorage(firstLocation.byteBuffer, capacity);
            break;
        case NextFreeSpot::ARCHETYPE_FULL: {
            const ByteBufferIndex bufferIndex = expands % MAX_STORAGES;
            capacity = std::max(capacity * 2, mem::round_up_to_64(count));
            expandAllocator(bufferIndex, capacity);
            firstLocation = EntityLocation(bufferIndex, storage.sizes[bufferIndex]);
            ++expands;
            break;
        }
        default: break;
    }
    storage.sizes[firstLocation.byteBuffer] += count;

    for (DataIndex i = 0; i < count; ++i) {
        auto& loc = metadata->at(entities[i])->location;
        loc.byteBuffer = firstLocation.byteBuffer;
        loc.dataIndex = firstLocation.dataIndex + i;
    }
    return firstLocation;
}

void * Archetype::getAt(const EntityLocation &loc, const TypeUUID typeID) {
    size_t hint = 0;
    return getAt(loc, typeID, hint);
}

void * Archetype::getAt(const EntityLocation &loc, const TypeUUID typeID, size_t &hint) {
    auto* type = findType(typeID, hint);

    if (!type) return nullptr;
    hint = type - storage.typeIndices;

    return type->typeInfo.index(type->chunks[loc.byteBuffer].data(), loc.dataIndex);
}

void * Archetype::getAtTracked(const EntityLocation &loc, const TypeUUID typeID, size_t &hint) {
    auto* type = findType(typeID, hint);

    if (!type) return nullptr;
    hint = type - storage.typeIndices;

    type->changes[loc.byteBuffer].set(loc.dataIndex);

    return type->typeInfo.index(type->chunks[loc.byteBuffer].data(), loc.dataIndex);
}

void * Archetype::getAtTracked(const EntityLocation &loc, const TypeUUID typeID) {
    size_t hint = 0;
    return getAtTracked(loc, typeID, hint);
}

void Archetype::initializeEntity(const EntityLocation& location, EntityTypeDataIterator iterator) const {
    for (auto [type, data] : iterator) {
        auto& typeIndex = findTypeIndex(type);
        auto& typeInfo = typeIndex.typeInfo;
        auto& chunk = typeIndex.chunks[location.byteBuffer];

        typeInfo.move(
            typeInfo.index(chunk.data(), location.dataIndex), data
        );

        if (typeIndex.enableChanges) {
            typeIndex.changes[location.byteBuffer].set(location.dataIndex);
        }
    }
}

void Archetype::eraseEntity(const Entity& e, const EntityLocation& loc) {
    const auto lastEntityIdx = --storage.sizes[loc.byteBuffer];

    for (int i = 0; i < storage.types; ++i) {
        auto& typeIndex = storage.typeIndices[i];
        const auto& type = typeIndex.typeInfo;
        char* buffer = typeIndex.chunks[loc.byteBuffer].data();

        void* source = type.index(buffer, lastEntityIdx);
        if (lastEntityIdx == loc.dataIndex) {
            type.destroy(source, 1);
        } else {
            void* dest = type.index(buffer, loc.dataIndex);
            type.move(dest, source);
            type.destroy(source, 1);
        }

        if (typeIndex.enableChanges) {
            typeIndex.changes[loc.byteBuffer].reset(loc.dataIndex);
        }
    }

    if (lastEntityIdx != loc.dataIndex) {
        Entity* entities = storage.entities[loc.byteBuffer];
        entities[loc.dataIndex] = entities[lastEntityIdx];
        metadata->at(entities[loc.dataIndex])->location.dataIndex = loc.dataIndex;
    }
    storage.lastIndex = loc.byteBuffer;
}

Archetype::TypeIndex* Archetype::findType(const TypeUUID type) {
    const auto begin = storage.typeIndices;
    const auto end = storage.typeIndices + storage.types;

    if (const auto it = std::ranges::lower_bound(
        begin, end, type, {}, &TypeIndex::type);
        it != end && it->type == type
    ) {
        return it;
    }
    return nullptr;
}

Archetype::TypeIndex* Archetype::findType(const TypeUUID type, const size_t hint) {
    if (hint >= storage.types) return nullptr;

    const auto begin = storage.typeIndices + hint;
    const auto end = storage.typeIndices + storage.types;

    if (const auto it = std::ranges::lower_bound(
        begin, end, type, {}, &TypeIndex::type);
        it != end && it->type == type
    ) {
        return it;
    }
    return nullptr;
}

Archetype::Archetype(Archetype &&other) noexcept: expands(other.expands), capacity(other.capacity), metadata(other.metadata), storage(std::move(other.storage)) {}

Archetype & Archetype::operator=(Archetype &&other) noexcept {
    if (this != &other) {
        this->~Archetype();
        new (this) Archetype(std::move(other));
    }
    return *this;
}

Archetype::~Archetype() {
    for (int i = 0; i < storage.types; ++i) {
        auto& typeIndex = storage.typeIndices[i];
        auto& typeInfo = typeIndex.typeInfo;

        for (int j = 0; j < MAX_STORAGES; ++j) {
            typeInfo.destroy(typeIndex.chunks[j].data(), storage.sizes[j]);
        }
    }
    delete[] storage.typeIndices;
}

void Archetype::overwriteEntity(const EntityLocation& loc, EntityTypeDataIterator iterator) {
    size_t lastType = 0;
    for (const auto& [type, newData] : iterator) {
        auto& typeIndex = findTypeIndex(type, lastType);
        auto typeInfo = typeIndex.typeInfo;

        auto& buffer = typeIndex.chunks[loc.byteBuffer];
        void* oldData = typeInfo.index(buffer.data(), loc.dataIndex);
        typeInfo.move(oldData, newData);
        lastType = &typeIndex - storage.typeIndices;

        if (typeIndex.enableChanges) {
            typeIndex.changes[loc.byteBuffer].set(loc.dataIndex);
        }
    }
}

void Archetype::addEntity(const Entity& e, EntityTypeDataIterator iterator) {
    const EntityLocation& loc = constructEntity(e);
    storage.entities[loc.byteBuffer][loc.dataIndex] = e;
    initializeEntity(loc, iterator);
}

void Archetype::addEntities(const Entity* entities, const DataIndex count, void** data) {
    auto loc = constructEntityMulti(entities, count);

    for (auto i = 0; i < storage.types; ++i) {
        TypeIndex& typeIndex = storage.typeIndices[i];
        const auto& typeInfo = typeIndex.typeInfo;

        void* dst = typeInfo.index(typeIndex.chunks[loc.byteBuffer].data(), loc.dataIndex);
        typeInfo.move(dst, data[i], count);

        if (typeIndex.enableChanges) {
            typeIndex.changes[loc.byteBuffer].set_range(loc.dataIndex, loc.dataIndex + count);
        }
    }
    Entity* ptr = &storage.entities[loc.byteBuffer][loc.dataIndex];
    std::memcpy(ptr, entities, sizeof(Entity) * count);
}

void Archetype::deleteEntity(const Entity& e) {
    const EntityLocation& loc = metadata->at(e)->location;
    eraseEntity(e, loc);
}

void Archetype::removeEntity(Archetype& dst, const Entity& e) {
    const EntityLocation prevLoc = metadata->at(e)->location;
    const EntityLocation& newLoc = dst.constructEntity(e);
    dst.storage.entities[newLoc.byteBuffer][newLoc.dataIndex] = e;

    for (int i = 0; i < dst.storage.types; ++i) {
        TypeIndex& typeIndex = findTypeIndex(dst.storage.typeIndices[i].type);
        const auto type = typeIndex.typeInfo;

        auto& buffer = typeIndex.chunks[prevLoc.byteBuffer];
        void* data = type.index(buffer.data(), prevLoc.dataIndex);

        const TypeIndex& dstTypeIndex = dst.storage.typeIndices[i];
        auto& dstBuffer = dstTypeIndex.chunks[newLoc.byteBuffer];
        type.move(
            type.index(dstBuffer.data(), newLoc.dataIndex), data
        );

        if (typeIndex.enableChanges) {
            typeIndex.changes[prevLoc.byteBuffer].reset(prevLoc.dataIndex);
        }
    }
    eraseEntity(e, prevLoc);
}

void Archetype::moveEntity(Archetype& dst, const Entity& e, EntityTypeDataIterator iterator) {
    cexpr::debug_assert([&]{
        return dst.storage.types > storage.types;
    });
    const EntityLocation prevLoc = metadata->at(e)->location;
    dst.addEntity(e, iterator);
    const EntityLocation& newLoc = metadata->at(e)->location;

    size_t lastTypeIndex = 0;

    auto archIterator = getTypeIterator();
    auto csIterator = iterator.toTypeIterator();

    for (auto type : set_difference_iterator(archIterator, csIterator)) { // TODO batching & precomputing the range
        TypeIndex& typeIndex = findTypeIndex(type, lastTypeIndex);
        auto typeInfo = typeIndex.typeInfo;
        auto& buffer = typeIndex.chunks[prevLoc.byteBuffer];

        void* data = typeInfo.index(buffer.data(), prevLoc.dataIndex);

        TypeIndex& dstTypeIndex = dst.findTypeIndex(type, lastTypeIndex);
        auto& dstBuffer = dstTypeIndex.chunks[newLoc.byteBuffer];
        typeInfo.move(
            typeInfo.index(dstBuffer.data(), newLoc.dataIndex), data
        );
        lastTypeIndex = &typeIndex - storage.typeIndices;

        if (typeIndex.enableChanges) {
            typeIndex.changes[prevLoc.byteBuffer].set(prevLoc.dataIndex);
        }
    }
    eraseEntity(e, prevLoc);
}

Archetype::TypeIndex& Archetype::findTypeIndex(const TypeUUID type, const size_t hint) const {
    TypeIndex* begin = storage.typeIndices + hint;
    TypeIndex* end = storage.typeIndices + storage.types;

    const auto it = std::ranges::lower_bound(
        begin, end, type, {}, &TypeIndex::type
    );
    cexpr::debug_assert([&]{
        return it != end;
    });
    return *it;
}

void Archetype::clearChanges() const {
    if (!storage.anyEnabledChanges) return;

    for (int i = 0; i < storage.types; ++i) {
        auto& typeIndex = storage.typeIndices[i];
        if (typeIndex.enableChanges) {
            typeIndex.changes[0].clear();
        }
    }
}

void Archetype::reset() {
    DataIndex highestIdx = 0;
    DataIndex highestCapacity = 0;

    for (int i = 0; i < MAX_STORAGES; ++i) {
        const auto size = storage.sizes[i];
        const auto currCapacity = storage.capacities[i];

        if (currCapacity > highestCapacity) {
            highestCapacity = currCapacity;
            highestIdx = i;
        }

        for (int j = 0; j < storage.types; ++j) {
            auto& typeIndex = storage.typeIndices[j];
            auto& chunk = typeIndex.chunks[i];
            auto& typeInfo = typeIndex.typeInfo;
            typeInfo.destroy(chunk.data(), size);
        }
        storage.sizes[i] = 0;
        storage.capacities[i] = 0;
        expands = 0;
    }

    for (int i = 0; i < MAX_STORAGES; ++i) {
        if (i == highestIdx) continue;

        for (int j = 0; j < storage.types; ++j) {
            storage.arenas[i] = {};
            storage.entities[i] = nullptr;

            for (int k = 0; k < storage.types; ++k) {
                auto& typeIndex = storage.typeIndices[k];
                typeIndex.chunks[i] = {};
                typeIndex.changes[i] = {};
            }
        }
    }

    if (highestIdx != 0) {
        storage.entities[0] = storage.entities[highestIdx];
        storage.arenas[0] = std::move(storage.arenas[highestIdx]);

        for (int i = 0; i < storage.types; ++i) {
            auto& typeIndex = storage.typeIndices[i];
            typeIndex.chunks[0] = typeIndex.chunks[highestIdx];
            typeIndex.changes[0] = std::move(typeIndex.changes[highestIdx]);

            typeIndex.chunks[highestIdx] = {};
        }
        storage.entities[highestIdx] = nullptr;
    }
    storage.capacities[0] = highestCapacity;

    storage.lastIndex = 0;
    capacity = highestCapacity;
}

template <typename Incl, typename Excl>
Archetype createSubsetArchetype(PrimaryKindRegistry& componentRegistry, EntityMetadataStorage<EntityMetadata>* metadata, Incl&& include, Excl&& exclude) {
    Archetype arch(metadata);

    mem::vector<TypeUUID> result;

    for (auto type : set_difference_iterator(include, exclude)) {
        result.emplace_back(type);
    }

    arch.allocateArchetype(result.size());

    for (auto type : result) {
        arch.allocateType(componentRegistry.getFieldOf(type), type);
    }
    return arch;
}

void ComponentStorage2::initializeArchetype(const size_t hash, Archetype &archetype, ArchetypeIndex index) {
    for (auto type : archetype.getTypeIterator()) {
        archIndices[type].emplace_back(index);
    }
    archHashes.emplace(hash, index);
}

TypeUUID ComponentStorage2::findBestID(const mem::range<TypeUUID> types) {
    TypeUUID bestID{};
    size_t minSize = std::numeric_limits<size_t>::max();

    for (TypeUUID type : types) {
        auto& val = archIndices[type];
        if (val.size() < minSize) {
            minSize = val.size();
            bestID = type;
        }
    }
    return bestID;
}

ComponentStorage2::ComponentStorage2(PrimaryKindRegistry* registry, EntityMetadataStorage<EntityMetadata>& metadata) : metadata(metadata), componentRegistry(registry) {
    archetypes.reserve(100);
    archetypes.emplace_back(createEmptyArchetype(&metadata));
}

void ComponentStorage2::clearArchetypeTrackedChanges() {
    for (auto& [archetype, edges] : archetypes) {
        archetype.clearChanges();
    }
}

Archetype& ComponentStorage2::getArchetype(const Entity& entity) {
    cexpr::require(metadata[entity].location.isValid());
    return archetypes[metadata[entity].location.archIndex].archetype;
}

void ComponentStorage2::deleteEntity(const Entity& e) {
    auto& location = metadata[e].location;
    if (!location.isValid()) return;
    ArchetypeIndex archIndex = location.archIndex;

    auto& [archetype, edges] = archetypes[archIndex];
    archetype.deleteEntity(e);
    location.clear();
}

ArchetypeIndex ComponentStorage2::findTargetArchetypeForEntity(EntityTypeIterator it, size_t addsHash) {
    ArchetypeIndex archIndex = 0;

    if (const auto archHashIt = archHashes.find(addsHash); archHashIt != archHashes.end()) {
        archIndex = archHashIt->second;
    } else {
        archIndex = static_cast<ArchetypeIndex>(archetypes.size());
        auto& [dstArchetype, edges] = archetypes.emplace_back(createArchetypeFromTypeIterator(*componentRegistry, &metadata, it));
        initializeArchetype(addsHash, dstArchetype, archIndex);
    }
    return archIndex;
}

ArchetypeIndex ComponentStorage2::findTargetArchetypeForEntity(ArchetypeIndex source,
    mem::range<TypeUUID> removes, EntityTypeIterator adds, const size_t removesHash, const size_t addsHash)
{
    auto& archetype = archetypes[source];

    ArchetypeTransitionKey key;
    key.adds = addsHash;
    key.removes = removesHash;

    ArchetypeIndex dstArchIndex = 0;

    if (auto dstIndex = archetype.findEdge(key); dstIndex != INVALID_INDEX<ArchetypeIndex>) {
        dstArchIndex = dstIndex;
    } else {
        auto unionIterator = set_union_iterator(archetype.archetype.getTypeIterator(), adds);
        auto diffIterator = set_difference_iterator(unionIterator, removes);

        const size_t totalHash = hash_unsigned_span(diffIterator);

        if (const auto it = archHashes.find(totalHash); it != archHashes.end()) {
            dstArchIndex = it->second;
        } else {
            auto uIt = set_union_iterator(archetype.archetype.getTypeIterator(), adds);
            const ArchetypeIndex index = static_cast<ArchetypeIndex>(archetypes.size());
            auto& [dstArchetype, dstEdges] = archetypes.emplace_back(createSubsetArchetype(*componentRegistry, &metadata, uIt, removes));
            initializeArchetype(totalHash, dstArchetype, index);
            dstArchIndex = index;
        }
        archetype.addEdge(key, dstArchIndex);
    }
    return dstArchIndex;
}

void ComponentStorage2::createEntities(const size_t hash, mem::range<TypeUUID> types, EntityCommandBuffer<CreateTag>* buffer) {
    ArchetypeIndex archIndex;
    if (const auto it = archHashes.find(hash); it != archHashes.end()) {
        auto& [arch, edges] = archetypes[it->second];

        arch.addEntities(buffer->entities, buffer->size, buffer->data);
        archIndex = it->second;
    } else {
        archIndex = static_cast<ArchetypeIndex>(archetypes.size());
        auto& [arch, edges] = archetypes.emplace_back(
            createArchetypeFromTypeIterator(*componentRegistry, &metadata, types)
        );
        initializeArchetype(hash, arch, archIndex);

        arch.addEntities(buffer->entities, buffer->size, buffer->data);
    }

    for (size_t i = 0; i < buffer->size; ++i) {
        metadata[buffer->entities[i]].location.archIndex = archIndex;
    }
}

void ComponentStorage2::processEntityBuffer(EntityDeferredOpsBuffer& buffer) {
    auto& finalEntities = buffer.finalEntities;

    if (finalEntities.empty()) return;

    int i = 0;

    bool finished = false;

    while (!finished) {
        auto srcArch = finalEntities[i].second.srcArch;
        auto dstArch = finalEntities[i].second.dstArch;

        int j = i;

        if (srcArch == INVALID_INDEX<ArchetypeIndex>) break;

        while (srcArch == finalEntities[i].second.srcArch && dstArch == finalEntities[i].second.dstArch) {
            i++;
            if (i == finalEntities.size()) {
                finished = true;
            }
        }

        auto& [srcArchetype, sedges] = archetypes[srcArch];
        auto& [dstArchetype, dEdges] = archetypes[dstArch];

        if (srcArch != dstArch) {
            if (finalEntities[j].second.adds.empty()) {
                for (; j < i; ++j) {
                    auto& entity = finalEntities[j].first;
                    srcArchetype.removeEntity(dstArchetype, entity);
                    metadata[entity].location.archIndex = dstArch;
                }
            } else {
                for (; j < i; ++j) {
                    auto& entity = finalEntities[j].first;
                    srcArchetype.moveEntity(dstArchetype, entity, createTypeDataIterator(finalEntities[j].second.adds));
                    metadata[entity].location.archIndex = dstArch;
                }
            }
        } else {
            for (; j < i; ++j) {
                auto& entity = finalEntities[j].first;
                srcArchetype.overwriteEntity(metadata[entity].location, createTypeDataIterator(finalEntities[j].second.adds));
            }
        }
    }

    while (true) {
        auto dstArch = finalEntities[i].second.dstArch;
        while (dstArch == finalEntities[i].second.dstArch) {
            auto& entity = finalEntities[i].first;
            auto& [archetype, edges] = archetypes[dstArch];
            archetype.addEntity(entity, createTypeDataIterator(finalEntities[i].second.adds));
            metadata[entity].location.archIndex = dstArch;
            ++i;

            if (i == finalEntities.size()) return;
        }
    }
}

void ComponentStorage2::reset() {
    for (auto& [archetype, edges] : archetypes) {
        archetype.reset();
    }
}

void SecondaryArchetype::initializeChunk(Chunk& chunk, const size_t withCapacity) const {
    mem::bytes_required req{};
    req.include(mem::type_info::of<Entity>(), withCapacity);
    req.include(type, withCapacity);

    chunk.allocator.initialize(req);
    chunk.capacity = withCapacity;
    chunk.entities = chunk.allocator.allocate<Entity>(withCapacity);
    chunk.buffer = static_cast<char*>(chunk.allocator.allocate(type, withCapacity));
}

// SecondaryEntityLocation.data will be invalid and is expensive to recompute - unused but kept in case
void SecondaryArchetype::expandChunk(Chunk &chunk, const size_t withCapacity) const {
    mem::bytes_required req{};
    req.include(mem::type_info::of<Entity>(), withCapacity);
    req.include(type, withCapacity);

    Arena newAllocator(req);
    auto* newEntities = newAllocator.allocate<Entity>(withCapacity);
    auto* newBuffer = static_cast<char*>(newAllocator.allocate(type, withCapacity));

    memcpy(newEntities, chunk.entities, chunk.size * sizeof(Entity));
    type.move(newBuffer, chunk.buffer, chunk.size);
    type.destroy(chunk.buffer, chunk.size);

    chunk.allocator = std::move(newAllocator);
    chunk.entities = newEntities;
    chunk.buffer = newBuffer;
    chunk.capacity = withCapacity;
}

std::tuple<ByteBufferIndex, void *, Entity *> SecondaryArchetype::nextFree(const size_t forEntities) {
    for (auto i = lastAvailableSpot; i < chunks.size(); ++i) {
        auto& [buffer, entities, allocator, size, capacity] = chunks[i];

        if (size + forEntities <= capacity) {
            lastAvailableSpot = i;
            return std::make_tuple(i, type.index(buffer, size), entities + size);
        }
    }
    return std::make_tuple(static_cast<ByteBufferIndex>(chunks.size()), nullptr, nullptr);
}

SecondaryArchetype::SecondaryArchetype(SecondaryArchetype &&other) noexcept: type(other.type), expands(other.expands), lastAvailableSpot(other.lastAvailableSpot), typeID(other.typeID) {
    for (auto i = 0; i < 10; ++i) {
        chunks[i] = std::move(other.chunks[i]);
    }
    other.expands = 0;
    other.lastAvailableSpot = 0;
    other.typeID = {};
    other.type = mem::type_info_of<void>;
}

SecondaryArchetype & SecondaryArchetype::operator=(SecondaryArchetype &&other) noexcept {
    if (this != &other) {
        expands = other.expands;
        lastAvailableSpot = other.lastAvailableSpot;
        typeID = other.typeID;
        type = other.type;

        for (auto i = 0; i < 10; ++i) {
            chunks[i] = std::move(other.chunks[i]);
        }
        other.expands = 0;
        other.lastAvailableSpot = 0;
        other.typeID = {};
        other.type = mem::type_info_of<void>;
    }
    return *this;
}

SecondaryArchetype::~SecondaryArchetype() {
    for (const auto& chunk : chunks) {
        if (chunk.size > 0) type.destroy(chunk.buffer, chunk.size);
    }
}

SecondaryArchetype SecondaryArchetype::create(const TypeUUID typeID, mem::typeindex type) {
    SecondaryArchetype arch;
    arch.type = type;
    arch.typeID = typeID;
    return arch;
}

ByteBufferIndex SecondaryArchetype::reserve(const size_t count) {
    auto [buffer, dataPtr, entityPtr] = nextFree(count);

    if (!dataPtr) {
        const auto expandedCapacity = static_cast<size_t>(std::pow(2, expands));
        const size_t newCapacity = std::max(count, expandedCapacity);
        chunks.emplace_back();
        initializeChunk(chunks[buffer], newCapacity);
        ++expands;
    }
    return buffer;
}

std::pair<ByteBufferIndex, void *> SecondaryArchetype::add(const Entity *entities, void **data, const size_t count) {
    auto [buffer, dataPtr, entityPtr] = nextFree(count);

    if (!dataPtr) {
        const auto expandedCapacity = static_cast<size_t>(std::pow(2, expands));
        const size_t newCapacity = std::max(count, expandedCapacity);

        if (buffer != 10) {
            initializeChunk(chunks[buffer], newCapacity);
            dataPtr = chunks[buffer].buffer;
            entityPtr = chunks[buffer].entities;
        } else {
            buffer = expands % 10;
            expandChunk(chunks[buffer], newCapacity);
            dataPtr = type.index(chunks[buffer].buffer, chunks[buffer].size);
            entityPtr = chunks[buffer].entities + chunks[buffer].size;
        }
        ++expands;
    }

    memcpy(entityPtr, entities, count * sizeof(Entity));
    type.move(dataPtr, data, count);
    chunks[buffer].size += count;

    return std::make_pair(buffer, dataPtr);
}

void SecondaryArchetype::construct(const Entity &entity, void *data, const ByteBufferIndex index) {
    auto& [buffer, entities, allocator, size, capacity] = chunks[index];

    memcpy(entities + size, &entity, sizeof(Entity));
    type.move(buffer, data, 1);
    ++size;
}

void SecondaryArchetype::remove(Metadata &metadata, const SecondaryEntityLocation &location) {
    auto& [buffer, entities, allocator, size, capacity] = chunks[location.bufferIndex];

    void* lastOffset = type.index(buffer, size - 1);
    type.destroy(location.data, 1);

    if (lastOffset != location.data) {
        type.move(location.data, lastOffset);
        type.destroy(lastOffset);

        auto& swappedEntity = metadata[entities[size - 1]];
        auto& swappedLocation = swappedEntity.findUnchecked(typeID);

        swappedLocation.data = location.data;
    }
    --size;
    lastAvailableSpot = std::min(lastAvailableSpot, location.bufferIndex);
}

void SecondaryArchetype::reset() {
    size_t bestCapacity = 0;
    size_t bestIndex = 0;

    for (size_t i = 0; i < chunks.size(); ++i) {
        auto& [buffer, entities, allocator, size, capacity] = chunks[i];
        type.destroy(buffer, size);
        size = 0;

        if (capacity > bestCapacity) {
            bestCapacity = capacity;
            bestIndex = i;
        }
    }

    if (bestIndex != 0) {
        std::swap(chunks[0], chunks[bestIndex]);
    }

    while (chunks.size() > 1) {
        chunks.pop_back();
    }
}

void SparseComponentStorage::add(const Entity *entities, const TypeUUID type, void *data, const size_t count) {
    auto& archetype = getOrCreateArchetype(type);
    const auto typeInfo = componentRegistry->getTypeInfoOf(type);

    auto index = archetype.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        auto& entity = entities[i];
        auto& location = findOrCreateLocation(entity, type);
        if (location.data) {
            typeInfo.move(location.data,
                  typeInfo.index(data, i)
            );
        } else {
            void* loc = typeInfo.index(data, i);
            archetype.construct(entity, loc, index);
            location.data = loc;
            location.bufferIndex = index;
        }
    }
}

void SparseComponentStorage::reset() {
    for (auto& archetype : archetypes) {
        archetype.reset();
    }
}

void BooleanStorage::deleteEntity(const Entity& entity) {
    for (const auto bit : metadata[entity].tags) {
        auto& archetype = getArchetype(TypeUUID::of(ComponentKind::of<BooleanComponentType>(), static_cast<uint16_t>(bit)));
        archetype.remove(&entity, 1);
    }
    metadata[entity].tags.free();
}

void BooleanStorage::add(const Entity* entities, const TypeUUID type, const size_t count) {
    auto& archetype = getOrCreateArchetype(type);

    const auto index = type.id();

    for (size_t i = 0; i < count; ++i) {
        metadata[entities[i]].tags.set_and_expand(index);
    }
    if (archetype.isActive()) archetype.add(entities, count);
}

void BooleanStorage::remove(const Entity* entities, const TypeUUID type, const size_t count) {
    auto& archetype = getOrCreateArchetype(type);
    const auto index = type.id();

    for (size_t i = 0; i < count; ++i) {
        metadata[entities[i]].tags.reset_checked(index);
    }
    if (archetype.isActive()) archetype.remove(entities, count);
}


EntityRegistry::EntityRegistry(EntityID initialCapacity, EntityID threadLocalCapacity)
    : defaultEntityCapacity(initialCapacity), threadLocalEntityCapacity(threadLocalCapacity), nextEntityID(1),
      metadata(initialCapacity) {
}

EntityID EntityRegistry::incrementEntityGeneration(const Entity entity) {
    auto gen = entity.gen();

    if (gen == Entity::MAX_GENERATION) {
        gen = 0;
    }
    gen = gen + 1;
    metadata[entity].generation = gen;
    return gen;
}

void EntityRegistry::deleteEntity(const Entity e) {
    if (e.id() != getLiveGeneration(e)) {
        return;
    }
    deletedEntitiesQueue.deleteEntity(e);
}

Entity EntityRegistry::create() {
    if (entityCreator.local().hasEntity()) {
        return entityCreator.local().getEntity();
    }
    const EntityID first = nextEntityID.fetch_add(threadLocalEntityCapacity, std::memory_order_relaxed);

    entityCreator.local().setSlice(first, first + threadLocalEntityCapacity);
    return entityCreator.local().getEntity();
}

void EntityRegistry::addComponentType(const AbstractComponentType &type) {
    const int kindInt = static_cast<int>(type.kind);

    if (kindInt >= components.size()) {
        components.resize(kindInt + 1);

        while (components.size() != components.capacity()) {
            components.emplace_back();
        }
    }
    components[kindInt] = type;
}

bool EntityRegistry::hasComponentType(const ComponentKind kind) const {
    if (components.size() <= static_cast<int>(kind)) return false;
    return components[static_cast<int>(kind)].instance != nullptr;
}

void EntityRegistry::reset() {
    nextEntityID = 1;

    for (auto& [next, cap, ids] : entityCreator) {
        cap = 0;
        ids.clear();
        next = 0;
    }

    for (auto& type : components) {
        // TODO onReset
    }
}

void EntityRegistry::onSynchronize(LevelContext &level) {
    const auto threads = entityCreator.size();

    if (!threads) return;

    deletedEntities.clear();
    deletedEntitiesQueue.merge(deletedEntities);

    const size_t perThread = deletedEntities.size() / threads;
    const size_t remaining = deletedEntities.size() % threads;

    size_t i = 0;
    for (auto& thread : entityCreator) {
        if (i == 0) {
            thread.ids.reserve(perThread + remaining);

            for (; i < remaining + perThread; ++i) {
                const Entity e = deletedEntities[i];
                const auto recycled = Entity(incrementEntityGeneration(e), e.id());
                thread.recycleEntity(recycled);
            }
        } else {
            thread.ids.reserve(perThread);

            for (; i < perThread; ++i) {
                const Entity e = deletedEntities[i];

                const auto recycled = Entity(incrementEntityGeneration(e), e.id());
                thread.recycleEntity(recycled);
            }
        }
    }
}
