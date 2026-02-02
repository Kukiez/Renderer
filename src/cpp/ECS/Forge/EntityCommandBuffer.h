#pragma once
#include <ECS/Entity/Entity.h>
#include "ECS/System/RuntimeSystemDescriptor.h"
#include <ECS/Component/Types/PrimaryKindRegistry.h>

template <typename T>
struct EntityCommandBuffer;

struct CreateTag {};
struct AppendTag {};
struct RemoveTag {};
struct DeleteTag {};
struct NameChangeTag {};
struct AddSystemTag {};
struct HierarchyLinkTag {};

class EntityCreateOps;
struct DeferredEntityOpsCenter;

template <>
struct EntityCommandBuffer<CreateTag> {
    size_t componentCount{};
    DataIndex size{};
    DataIndex capacity{};
    Entity* entities{};
    void** data{};

    static EntityCommandBuffer construct(size_t count, const DataIndex capacity, void** arr, Entity* entities) {
        EntityCommandBuffer buffer;
        buffer.componentCount = count;
        buffer.size = 0;
        buffer.capacity = capacity;
        buffer.entities = entities;
        buffer.data = arr;
        return buffer;
    }

    DataIndex current() const {
        return size;
    }
    
    DataIndex remaining() const {
        return capacity - size;
    }
};

template <> struct EntityCommandBuffer<AppendTag> {
    Entity entity;
    TypeUUID type;
    void* data;

    bool operator==(const auto& other) const {
        return std::tie(entity, type) == std::tie(other.entity, other.type);
    }

    bool operator<(const auto& other) const {
        if (entity != other.entity) {
            return entity < other.entity;
        }
        return type < other.type;
    }
};

template <> struct EntityCommandBuffer<RemoveTag> {
    Entity entity;
    TypeUUID type;

    bool operator==(const auto& other) const {
        return std::tie(entity, type) == std::tie(other.entity, other.type);
    }

    bool operator<(const auto& other) const {
        if (entity != other.entity) {
            return entity < other.entity;
        }
        return type < other.type;
    }
};

template <> struct EntityCommandBuffer<DeleteTag> {
    Entity entity;

    bool operator == (const auto& other) const {
        return entity == other.entity;
    }

    bool operator < (const auto& other) const {
        return entity < other.entity;
    }
};

template <> struct EntityCommandBuffer<HierarchyLinkTag> {
    Entity parent;
    Entity child;
};


template <> struct EntityCommandBuffer<NameChangeTag> {
    Entity entity;
    std::string_view name;
};

template <> struct EntityCommandBuffer<AddSystemTag> {
    RuntimeSystemDescriptor* system;
};

using AppendCommandBuffer = EntityCommandBuffer<AppendTag>;
using RemoveCommandBuffer = EntityCommandBuffer<RemoveTag>;
using DeleteCommandBuffer = EntityCommandBuffer<DeleteTag>;
using CreateCommandBuffer = EntityCommandBuffer<CreateTag>;
using HierarchyLinkCommandBuffer = EntityCommandBuffer<HierarchyLinkTag>;
using HierarchyBreakCommandBuffer = EntityCommandBuffer<HierarchyLinkTag>;
using NameChangeCommandBuffer = EntityCommandBuffer<NameChangeTag>;
using AddSystemCommandBuffer = EntityCommandBuffer<AddSystemTag>;

struct EntityDeferredOps {
    using Add = std::pair<TypeUUID, void*>;

    mem::vector<Add, mem::byte_arena_adaptor<Add>> adds;
    
    ArchetypeIndex dstArch = 0;
    ArchetypeIndex srcArch = 0;

    explicit EntityDeferredOps(mem::byte_arena<>& arena) : adds(&arena) {}

    bool operator == (const auto& other) const {
        return srcArch == other.srcArch && dstArch == other.dstArch;
    }

    bool operator != (const auto& other) const {
        return !(*this == other);
    }

    bool operator < (const auto& other) const {
        return std::tie(srcArch, dstArch) < std::tie(srcArch, dstArch);
    }
};

struct EntityDeferredOpsBuffer {
    using DeferredOpPair = std::pair<Entity, EntityDeferredOps>;

    mem::byte_arena<> arena = mem::create_byte_arena(1 * 1'024 * 1'024);
    mem::vector<DeleteCommandBuffer> deletedEntities;
    mem::vector<DeferredOpPair, mem::byte_arena_adaptor<DeferredOpPair>> finalEntities{&arena};

    void reserve(const size_t entities) {
        finalEntities.reserve(entities);
    }

    auto& emplace_back(const Entity& entity, const size_t addsLen) {
        auto& res = finalEntities.emplace_back(entity, arena);
        res.second.adds.reserve(addsLen);
        return res.second;
    }

    void reset() {
        arena.reset_compact();
        finalEntities.clear();
    }
};