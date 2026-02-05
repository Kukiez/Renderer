#pragma once
#include "ECS/ThreadLocal.h"
#include "ECS/Component/ComponentMap.h"
#include "ECS/Entity/Entity.h"
#include "memory/typeless_vector.h"
#include "memory/type_info.h"

class SecondaryStagingBuffer {
    using Arena = mem::byte_arena<mem::same_alloc_schema, 64>;

    struct DataBuffer {
        mem::typeindex type = mem::type_info_of<void>;
        void* dataPtr = nullptr;
        size_t size = 0;
        size_t capacity = 0;
        mem::vector<Entity, Arena::Adaptor<Entity>> entities;

        DataBuffer() = default;

        DataBuffer(Arena& allocator, mem::typeindex type)
        : type(type), entities(&allocator) {}

        void release() {
            if (dataPtr) {
                type.destroy(dataPtr, size);
                size = 0;
                capacity = 0;
                dataPtr = nullptr;
            }
            entities.release();
        }

        template <typename T>
        std::decay_t<T>& add(const Entity& entity, T&& component) {
            using Type = std::decay_t<T>;
            static constexpr auto TypeInfo = mem::type_info_of<T>;

            if (size == capacity) {
                auto& allocator = entities.get_allocator();

                void* newMem = allocator.arena->allocate(TypeInfo, capacity * 2 + 8);

                if (size != 0) TypeInfo.move(newMem, dataPtr);

                dataPtr = newMem;
                capacity = capacity * 2 + 8;
            }
            Type* dataTPtr = static_cast<Type*>(dataPtr);

            auto result = new (dataTPtr + size) Type(std::forward<T>(component));
            entities.emplace_back(entity);
            ++size;
            return *result;
        }
    };

    struct Ops {
        Arena arena = Arena(static_cast<size_t>(0.1 * 1024 * 1024));
        ComponentMap<DataBuffer> newComponents;
        ComponentMap<mem::vector<Entity, Arena::Adaptor<Entity>>> removedComponents;
    };
    ComponentKindRegistry<SecondaryField>* componentRegistry{};
    ThreadLocal<Ops> ops;
public:
    explicit SecondaryStagingBuffer(ComponentKindRegistry<SecondaryField>* registry) : componentRegistry(registry) {}

    template <typename T>
    void add(const Entity& entity, T&& component) {
        const auto type = componentRegistry->getTypeID<T>();
        DataBuffer& buffer = ops.local().newComponents.getOrCreate(type, ops.local().arena, mem::type_info_of<T>);
        buffer.add(entity, std::forward<T>(component));
    }

    template <typename T>
    void remove(const Entity& entity) {
        const auto type = componentRegistry->getTypeID<T>();
        ops.local().removedComponents.getOrCreate(type, ops.local().arena).emplace_back(entity);
    }

    void reset() {
        for (auto& [arena, newComponents, removedComponents] : this->ops) {
            for (auto& data : newComponents) {
                data.release();
            }
            for (auto& data : removedComponents) {
                data.release();
            }
            arena.reset_compact();
        }
    }

    auto begin() {
        return ops.begin();
    }

    auto end() {
        return ops.end();
    }
};