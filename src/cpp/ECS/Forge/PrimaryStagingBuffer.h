#pragma once
#include <ranges>

#include "EntityCommandBuffer.h"
#include "ECS/ThreadLocal.h"
#include "ECS/Component/ComponentMap.h"
#include "ECS/Component/Types/PrimaryKindRegistry.h"
#include "EntityCreateBuffer.h"

class EntityCreateOps {
public:
    using MapType = EntityCommandBuffer<CreateTag>*;
    using Arena = mem::byte_arena<mem::same_alloc_schema, 64>;

    struct CommandBufferDescriptor {
        mem::range<TypeUUID> types;
        mem::range<int> indices;
        mem::vector<MapType, mem::byte_arena_adaptor<MapType, Arena>> buffers;

        CommandBufferDescriptor() = default;
        CommandBufferDescriptor(mem::range<TypeUUID> types, mem::range<int> indices, Arena* arena)
            : types(types), indices(indices), buffers(arena) {}
    };
private:
    Arena allocator = mem::create_byte_arena<mem::same_alloc_schema, 64>(mem::megabyte(0.2).bytes());

    PrimaryKindRegistry* componentRegistry;
    std::unordered_map<size_t, CommandBufferDescriptor> entityCreateBuffers;

    template <typename... Ts>
    auto& getBuffer() {
        constexpr static size_t Hash = cexpr::pack_stable_hash_v<Ts...>;

        if (const auto it = entityCreateBuffers.find(Hash); it != entityCreateBuffers.end()) {
            return it->second;
        }
        const PrimaryTypeCache& cache = componentRegistry->getTypeCache<Ts...>();
        auto& indices = cache.typenameSortedIndices;
        auto& types = cache.sortedTypes;

        return entityCreateBuffers.emplace(Hash,
            CommandBufferDescriptor(
                mem::make_range(types, cache.count), mem::make_range(indices, cache.count), &allocator
            )
        ).first->second;
    }

    MapType createCommandBuffer(CommandBufferDescriptor& bufferDescriptor, size_t neededSpace, size_t ComponentCount);
    MapType getFreeCommandBuffer(CommandBufferDescriptor& bufferDescriptor, size_t minCapacity, size_t ComponentCount);
public:
    void DEBUG();

    EntityCreateOps(PrimaryKindRegistry* registry) : componentRegistry(registry) {}

    template <typename... Ts>
    auto createEntity(const Entity& entity, Ts&&... ts) {
        using Sorted = cexpr::sort_to_tuple_t<std::decay_t<Ts>...>;

        return cexpr::for_each_typename_in_tuple<Sorted>([&]<typename... Ss>(){
            auto& bufferDescriptor = getBuffer<Ss...>();
            auto* cmdBuffer = getFreeCommandBuffer(bufferDescriptor, 1, sizeof...(Ss));

            auto result = EntityCreateBuffer<Ss...>(
                cmdBuffer, bufferDescriptor.indices.data()
            ).createEntity(entity, std::forward<Ts>(ts)...);

            DEBUG();
            return result;
        });
    }

    auto& getEntityCreateBuffers() {
        return entityCreateBuffers;
    }

    void reset();
};

struct ComponentStagingMemoryBuffer {
    using DestructorFn = void(*)(void*, size_t N);

    mem::vector<char*> buffers;
    DestructorFn destructor = nullptr;
    size_t capacity = 0;
    size_t size = 0;

    ComponentStagingMemoryBuffer() = default;
    ComponentStagingMemoryBuffer(DestructorFn destructor) : destructor(destructor) {}

    bool hasSpace() const {
        if (buffers.empty()) [[unlikely]] return false;
        if (size == capacity) {
            return false;
        }
        return true;
    }

    void allocateNew(void* memory, size_t capacity) {
        this->capacity = capacity;
        this->size = 0;
        buffers.emplace_back(static_cast<char*>(memory));
    }

    template <typename C>
    void* add(C&& c) {
        using DC = std::decay_t<C>;
        DC* asC = reinterpret_cast<DC*>(buffers.back());

        return new (asC + size++) DC(std::forward<C>(c));
    }

    void reset() {
        if (!destructor || buffers.empty()) {
            buffers.clear();
            size = 0;
            capacity = 0;
            return;
        }
        destructor(buffers.back(), size);

        const size_t total = buffers.size() - 1;
        for (int i = 0; i < total; ++i) {
            const size_t pastSize = static_cast<size_t>(capacity / (std::pow(2, total - i)));
            destructor(buffers[i], pastSize);
        }
        buffers.clear();
        size = 0;
        capacity = 0;
    }

    auto getCapacity() const {
        return capacity;
    }
};

class DeferredEntityForge {
    PrimaryKindRegistry* componentRegistry{};

    mem::byte_arena<> arena = mem::create_byte_arena(1 * 1024 * 1024);
    ComponentMap<ComponentStagingMemoryBuffer> componentMemoryBuffer;

    mem::vector<AppendCommandBuffer, mem::byte_arena_adaptor<AppendCommandBuffer>> entityAppends{&arena};
    mem::vector<RemoveCommandBuffer, mem::byte_arena_adaptor<RemoveCommandBuffer>> entityRemoves{&arena};

    template <typename C>
    void addSingle(const Entity e, C&& c) {
        using CD = std::decay_t<C>;

        const TypeUUID type = componentRegistry->getTypeID<std::decay_t<C>>();
        auto buildInfo = [&]{
            if (auto* it = componentMemoryBuffer.find(type)) {
                return it;
            }
            if constexpr (std::is_trivially_destructible_v<std::decay_t<C>>)
                componentMemoryBuffer.emplace(type);
            else
                componentMemoryBuffer.emplace(type, [](void* s, size_t count){
                    CD* asC = static_cast<CD*>(s);
                    for (int i = 0; i < count; ++i) {
                        asC[i].~CD();
                    }
                });
            componentMemoryBuffer[type].allocateNew(arena.allocate(mem::type_info_of<CD>, 16), 16);
            return &componentMemoryBuffer[type];
        }();
        if (!buildInfo->hasSpace()) {
            size_t capacity = buildInfo->capacity * 2;
            buildInfo->allocateNew(arena.allocate(mem::type_info_of<CD>, capacity), capacity);
        }
        void* payload = buildInfo->add(std::forward<C>(c));
        entityAppends.emplace_back(e, type, payload);
    }
public:
    DeferredEntityForge() = default;

    explicit DeferredEntityForge(PrimaryKindRegistry* componentRegistry) : componentRegistry(componentRegistry) {}

    template <typename... Cs> // -> Components
    void add(const Entity e, Cs&&... args) {
        (addSingle<Cs>(e, std::forward<Cs>(args)), ...);
    }

    template <typename... Cs>
    void remove(const Entity& e) {
        (entityRemoves.emplace_back(e, componentRegistry->getTypeID<Cs>()), ...);
    }

    auto& getAppends() {
        return entityAppends;
    }

    auto& getRemovals() {
        return entityRemoves;
    }

    void reset() {
        arena.reset_compact();
        entityAppends.release();
        entityRemoves.release();

        entityAppends.reserve(8);
        entityRemoves.reserve(8);

        for (auto& stagingBuffer : componentMemoryBuffer) {
            stagingBuffer.reset();
        }
    }
};

class PrimaryStagingBuffer {
    PrimaryKindRegistry* componentRegistry{};

    ThreadLocal<DeferredEntityForge> primaryOps;
    ThreadLocal<EntityCreateOps> entityCreateOps;
public:
    explicit PrimaryStagingBuffer(PrimaryKindRegistry* componentRegistry)
    : componentRegistry(componentRegistry), primaryOps(componentRegistry), entityCreateOps(componentRegistry) {}

    template <typename... Ts>
    auto create(const Entity& entity, Ts&&... components) {
        return entityCreateOps.local().createEntity(entity, std::forward<Ts>(components)...);
    }

    template <typename... Ts>
    void add(const Entity& entity, Ts&&... components) {
        primaryOps.local().add(entity, std::forward<Ts>(components)...);
    }

    template <typename... Ts>
    void remove(const Entity& entity) {
        primaryOps.local().remove<Ts...>(entity);
    }

    auto& getPrimaryOps() {
        return primaryOps;
    }

    auto& getEntityCreateOps() {
        return entityCreateOps;
    }

    void reset() {
        for (auto& ops : primaryOps) {
            ops.reset();
        }
        for (auto& ops : entityCreateOps) {
            ops.reset();
        }
    }
};

template <typename YieldMode>
struct EntityTypeDataIteratorImpl {
    using value_type = YieldMode;
    std::pair<TypeUUID, void*>* first;
    std::pair<TypeUUID, void*>* last;

    bool operator == (const auto& other) const {
        return first == other.first;
    }

    bool operator != (const auto& other) const {
        return first != other.first;
    }

    const YieldMode& operator * () {
        if constexpr (std::is_same_v<TypeUUID, YieldMode>) {
            return first->first;
        } else {
            return *first;
        }
    }

    auto& operator ++() {
        ++first;
        return *this;
    }

    auto operator ++(int) {
        auto copy = *this;
        ++(*this);
        return copy;
    }

    auto begin() {
        return *this;
    }

    auto end() {
        auto copy = *this;
        copy.first = copy.last;
        return copy;
    }
};


struct EntityTypeIterator : public EntityTypeDataIteratorImpl<TypeUUID> {};

struct EntityTypeDataIterator : public EntityTypeDataIteratorImpl<std::pair<TypeUUID, void*>> {
    auto toTypeIterator() const {
        EntityTypeIterator result;
        memcpy(&result, this, sizeof(*this));
        return result;
    }
};

inline EntityTypeDataIterator createTypeDataIterator(auto& cmds) {
    return EntityTypeDataIterator{
        cmds.data(), cmds.data() + cmds.size()
    };
}