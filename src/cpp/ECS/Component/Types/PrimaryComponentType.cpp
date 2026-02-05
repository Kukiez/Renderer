#include "PrimaryComponentType.h"

#include "SecondaryComponentType.h"
#include "BooleanComponentType.h"
#include "NameComponentType.h"
#include "ComponentFactory.h"
#include "ECS/Level/LevelContext.h"

const PrimaryTypeCache & PrimaryKindRegistry::createTypeCache(const size_t hash, const TypeUUID *types,
    const int count)
{
    PrimaryTypeCache cache;
    cache.count = count;
    cache.sortedTypes = new TypeUUID[cache.count];

    memcpy(cache.sortedTypes, types, count * sizeof(TypeUUID));

    std::ranges::sort(cache.sortedTypes, cache.sortedTypes + cache.count);

    cache.typenameSortedIndices = new int[cache.count];

    for (size_t i = 0; i < cache.count; ++i) {
        for (size_t j = 0; j < cache.count; ++j) {
            if (cache.sortedTypes[i] == types[j]) {
                cache.typenameSortedIndices[j] = static_cast<int>(i);
                break;
            }
        }
    }
    tbb::concurrent_hash_map<size_t, PrimaryTypeCache>::accessor accessor;
    rangesMap.emplace(accessor, hash, cache);
    return accessor->second;
}

void PrimaryComponentType::onSynchronize(LevelContext &level) {
    if (level.registry.getEntityLimit() > metadata.getCapacity()) {
        metadata.expand(level.registry.getEntityLimit());
    }
    componentRegistry.flushNewFields();

    auto& ops = staging;

    entityAppends.clear();
    entityRemovals.clear();

    size_t prefixSumAppends = 0;
    size_t prefixSumRemovals = 0;

    for (auto& tlOps : ops.getPrimaryOps()) {
        auto& primaryOps = tlOps;

        prefixSumAppends += primaryOps.getAppends().size();
        prefixSumRemovals += primaryOps.getRemovals().size();
    }

    auto sortContainer = [&]<typename Proj>(auto& vec, Proj&& proj) {
        for (auto& tlOps : ops.getPrimaryOps()) {
            auto& primaryOps = tlOps;
            vec.insert(vec.end(), (primaryOps.*proj)());
        }
        if (vec.empty()) return;

        std::ranges::sort(vec, [](const auto& a, const auto& b){
            return a < b;
        });
    };

    auto& buffer = stagingBuffer;

    buffer.reset();
    buffer.reserve(prefixSumAppends);

    entityAppends.reserve(prefixSumAppends);
    entityRemovals.reserve(prefixSumRemovals);

    sortContainer(entityAppends, &DeferredEntityForge::getAppends);
    sortContainer(entityRemovals, &DeferredEntityForge::getRemovals);

    readyChanges();

    commitEntityCreations();
    storage.processEntityBuffer(buffer);

    for (auto deleted : level.registry.getDeletedEntities()) {
        storage.deleteEntity(deleted);
    }
    staging.reset();
}

void PrimaryComponentType::onFrameEnd(LevelContext &level) {
    storage.clearArchetypeTrackedChanges();
}

void PrimaryComponentType::readyChanges() {
    removes.clear();
    removes.reserve(15);

    auto addsBegin = entityAppends.begin();
    auto removesBegin = entityRemovals.begin();

    const auto addsEnd = entityAppends.end();
    const auto removesEnd = entityRemovals.end();

    while (addsBegin != addsEnd && addsBegin->entity == NullEntity) {}
    while (removesBegin != removesEnd && removesBegin->entity == NullEntity) {}

    while (addsBegin != addsEnd || removesBegin != removesEnd) {
        Entity entity = Entity::max();

        const bool hasAdd = addsBegin != addsEnd;
        const bool hasRemove = removesBegin != removesEnd;

        if (hasAdd) {
            entity = addsBegin->entity;
        }

        if (hasRemove) {
            entity = std::min(entity, removesBegin->entity);
        }


        auto addsFirst = addsBegin;
        auto removesFirst = removesBegin;

        while (addsBegin    != addsEnd && addsBegin->entity    == entity) ++addsBegin;
        while (removesBegin != removesEnd && removesBegin->entity == entity) ++removesBegin;


        auto& ops = stagingBuffer.emplace_back(entity, addsBegin - addsFirst);

        size_t hashAdd = hash::DEFAULT_HASH;
        size_t hashRemove = hash::DEFAULT_HASH;

        bool isNew = !storage.hasComponents(entity);
        Archetype* archetype = !isNew ? &storage.getArchetype(entity) : (Archetype*)0;
        size_t hint = 0;

        auto addComponent = [&](auto& add) {
            ops.adds.emplace_back(add.type, add.data);

            if (isNew) {
                hash::hashNextType(add.type, hashAdd);
                return;
            }
            auto* typeIndex = archetype->findType(add.type, hint);

            if (typeIndex) {
                hint = archetype->indexOf(typeIndex);
            } else {
                hash::hashNextType(add.type, hashAdd);
            }
        };

        auto removeComponent = [&](TypeUUID type) {
            // removals are sorted, if N was not found N + 1 will not either
            if (isNew) {
                removesFirst = removesBegin - 1;
                return;
            }
            auto* typeIndex = archetype->findType(type, hint);

            if (!typeIndex) {
                removesFirst = removesBegin - 1;
                return;
            }
            hint = archetype->indexOf(typeIndex);
            hash::hashNextType(type, hashRemove);
            removes.emplace_back(type);
        };

        auto advanceWhileSameType = [](auto& it, const auto& max) {
            TypeUUID type = it->type;
            do {
                ++it;
            } while (it != max && type == it->type);
        };

        while (removesFirst != removesBegin && addsFirst != addsBegin) {
            if (addsFirst->type < removesFirst->type) {
                advanceWhileSameType(addsFirst, addsBegin);
                addComponent(addsFirst[-1]);
            } else if (addsFirst->type > removesFirst->type) {
                removeComponent(removesFirst->type);
                advanceWhileSameType(removesFirst, removesBegin);
            } else {
                removeComponent(removesFirst->type);

                advanceWhileSameType(addsFirst, addsBegin);
                advanceWhileSameType(removesFirst, removesBegin);
            }
        }

        while (addsFirst != addsBegin) {
            advanceWhileSameType(addsFirst, addsBegin);
            addComponent(addsFirst[-1]);
        }

        while (removesFirst != removesBegin) {
            removeComponent(removesFirst->type);
            advanceWhileSameType(removesFirst, removesBegin);
        }

        ops.srcArch = storage.getMetadataOf(entity).location.archIndex;
        auto it = createTypeDataIterator(ops.adds).toTypeIterator();
        if (isNew) {
            ops.dstArch = storage.findTargetArchetypeForEntity(it, hashAdd);
        } else {
            ops.dstArch = storage.findTargetArchetypeForEntity(ops.srcArch, removes, it, hashRemove, hashAdd);
        }
        removes.clear();
    }

    std::ranges::sort(stagingBuffer.finalEntities, [](const auto& a, const auto& b){
        return a < b;
    });
}

void PrimaryComponentType::commitEntityCreations() {
    for (auto& tlOps : staging.getEntityCreateOps()) {
        for (auto& [hash, op] : tlOps.getEntityCreateBuffers()) {
            auto& [types, indices, commands] = op;

            for (auto& cmd : commands) {
                if (cmd->size == 0) continue;
                storage.createEntities(hash, types, cmd);
            }
        }
    }
}

void PrimaryComponentType::dumpArchetypes() {
    int i = 0;
    for (auto& [archetype, edges] : storage.getArchetypes()) {
        if (i == 0) {
            ++i;
            continue;
        }
        std::cout << "Archetype[" << i++ << "]\n";

        size_t entities = [&] {
            size_t s = 0;
            for (auto i = 0; i < 10; ++i) {
                s += archetype.storage.sizes[i];
            }
            return s;
        }();

        std::cout << "  Entities: " << entities << "\n";
        std::cout << "  Components: [ \n   ";
        for (auto type : archetype.getTypeIterator()) {
            std::cout << componentRegistry.getTypeInfoOf(type).name() << ", ";
        }
        std::cout << "\n  ],\n";
    }
    std::cout << std::endl;
}

ComponentFactory::ComponentFactory(LevelContext &level): registry(level.registry),
primary(level.registry.getComponentType<PrimaryComponentType>()), secondary(level.registry.getComponentType<SecondaryComponentType>()),
boolean(level.registry.getComponentType<BooleanComponentType>()), nameComponent(level.registry.getComponentType<NameComponentType>())
{
}

PrimaryEntityQueryData::PrimaryEntityQueryData(LevelContext &level) : storage(level.registry.getComponentType<PrimaryComponentType>()){
}

EntityCreateOps::MapType EntityCreateOps::createCommandBuffer(CommandBufferDescriptor& bufferDescriptor, const size_t neededSpace, const size_t ComponentCount) {
    auto& [types, indices, buffers] = bufferDescriptor;

    const DataIndex capacity = static_cast<DataIndex>((buffers.empty() ? 8 : buffers.back()->capacity * 2) + neededSpace);

    auto componentDataContainer = static_cast<void**>(allocator.allocate(
        mem::type_info_of<void*>, ComponentCount)
    );

    for (int i = 0; i < ComponentCount; ++i) {
        const auto type = componentRegistry->getTypeInfoOf(types[i]);
        componentDataContainer[i] = allocator.allocate(type, capacity);
    }

    const auto entityContainer = allocator.allocate<Entity>(capacity);

    auto* commandBuffer = allocator.allocate<CreateCommandBuffer>(1);

    new (commandBuffer) CreateCommandBuffer(CreateCommandBuffer::construct(
        ComponentCount, capacity, componentDataContainer,
        entityContainer
    ));
    return buffers.emplace_back(commandBuffer);
}

EntityCreateOps::MapType EntityCreateOps::getFreeCommandBuffer(CommandBufferDescriptor& bufferDescriptor, const size_t minCapacity, const size_t ComponentCount) {
    for (const auto& cmdBuffer : bufferDescriptor.buffers) {
        if (cmdBuffer->remaining() >= minCapacity) {
            return cmdBuffer;
        }
    }
    return createCommandBuffer(bufferDescriptor, minCapacity, ComponentCount);
}

void EntityCreateOps::reset() {
    DEBUG();

    for (auto& [types, indices, commands] : entityCreateBuffers | std::views::values) {
        for (auto* cmd : commands) {
            for (size_t i = 0; i < types.size(); ++i) {
                void* data = cmd->data[i];
                const TypeUUID type = types[i];
                const auto typeInfo = componentRegistry->getTypeInfoOf(type);
                typeInfo.destroy(data, cmd->size);
            }
        }
        commands.release();
    }
    allocator.reset_compact();
}

void EntityCreateOps::DEBUG() {
}