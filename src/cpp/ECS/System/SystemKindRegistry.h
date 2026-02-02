#pragma once

#include <ECS/Component/ComponentRegistry.h>
#include "RuntimeSystemDescriptor.h"
#include "StageKindRegistry.h"

struct SystemField {
    void* instance = nullptr;
    RuntimeSystemDescriptor* descriptor = nullptr;

    template <typename System>
    static SystemField of() {
        return { nullptr, nullptr };
    }

    template <typename System>
    static SystemField of(System* system, RuntimeSystemDescriptor* descriptor) {
        return { system, descriptor };
    }

    operator bool() const {
        return instance != nullptr;
    }
};

class SystemComponentType : public ComponentType<SystemComponentType> {};

struct SystemComponent {
    using ComponentType = SystemComponentType;
};

template <typename S>
struct _Wrapper : SystemComponent {};

class SystemKindRegistry {
    mem::byte_arena<mem::same_alloc_schema, 64> systemAllocator;
    mem::byte_arena<mem::same_alloc_schema, 64> systemStageDescriptorAllocator;
    mem::byte_arena<mem::same_alloc_schema, 64> systemDescriptorAllocator;
    mem::byte_arena<mem::same_alloc_schema, 64> dependencyAllocator;

    ComponentKind kind;

    std::vector<ComponentField<SystemField>> systemFields;
    std::vector<int> componentIndexToFieldIndex;

    template <typename System>
    ComponentIndex getComponentIndex() const {
        if constexpr (std::is_base_of_v<SystemComponent, System>) {
            return ComponentTypeRegistry::getOrCreateComponentIndex<System>(kind);
        } else {
            auto cIndex = ComponentTypeRegistry::getOrCreateComponentIndex<_Wrapper<std::decay_t<System>>>(kind);
            return cIndex;
        }
    }
public:
    explicit SystemKindRegistry(const ComponentKind kind) : kind(kind) {
        systemFields.emplace_back(ComponentField<SystemField>::of<NullSystem>());
    }

    SystemKindRegistry(const SystemKindRegistry&) = delete;
    SystemKindRegistry(SystemKindRegistry&&) = delete;

    ~SystemKindRegistry() {
        int i = 0;
        for (const auto& field : systemFields) {
            if (i == 0) {
                i = 1;
                continue;
            }
            mem::destroy_at(field.descriptor->type, field.instance, 1);

            for (auto& [stage, system] : field.descriptor->stages()) {
                system.~UpdateSystemDescriptor();
            }
            field.descriptor->~RuntimeSystemDescriptor();
        }
    }

    template <typename System>
    TypeUUID forwardDeclareSystem() {
        auto type = isDeclared<System>();
        if (type.id() != 0) {
            return type;
        }
        const auto cIndex = getComponentIndex<System>();

        componentIndexToFieldIndex.reserve(cIndex.id() + 1);
        while (componentIndexToFieldIndex.size() != componentIndexToFieldIndex.capacity()) {
            componentIndexToFieldIndex.emplace_back(0);
        }
        const auto id = static_cast<int>(systemFields.size());

        componentIndexToFieldIndex[cIndex.id()] = id;
        systemFields.emplace_back(ComponentField<SystemField>::of<System>());
        return TypeUUID::of(kind, componentIndexToFieldIndex[cIndex.id()]);
    }

    template <typename TypeTuple>
    TypeUUID* fillSystemStageDescriptor() {
        auto* types = dependencyAllocator.allocate<TypeUUID>(std::tuple_size_v<TypeTuple>);

        cexpr::for_each_typename_and_index_in_tuple<TypeTuple>(
            [&]<typename... Deps, size_t... Ds>(std::index_sequence<Ds...>)
        {
            ((types[Ds] = forwardDeclareSystem<Deps>()), ...);
        });
        return types;
    }

    template <typename System, typename Descriptor>
    bool addSystem(StageKindRegistry& stageRegistry) {
        const auto cIndex = getComponentIndex<System>();

        bool result = true;

        auto createSystem = [&] {
            System* system = systemAllocator.allocate<System>(1);
            new (system) System();

            auto descriptor = systemDescriptorAllocator.allocate<RuntimeSystemDescriptor>(1);

            int id;

            if (const auto declaredType = isDeclared<System>(); declaredType.id() != 0) {
                id = declaredType.id();
                systemFields[id] = ComponentField<SystemField>::of<System>(system, descriptor);
            } else {
                id = static_cast<int>(systemFields.size());
                systemFields.emplace_back(ComponentField<SystemField>::of<System>(system, descriptor));
                componentIndexToFieldIndex[cIndex.id()] = id;
            }

            size_t memIdx = 0;

            new (descriptor) RuntimeSystemDescriptor(RuntimeSystemDescriptor::of<System>());

            auto mem = systemStageDescriptorAllocator.allocate<RuntimeSystemStageDescriptor>(Descriptor::StageCount);

            cexpr::for_each_typename_and_index_in_tuple<typename Descriptor::stages>([&]<typename... Stage, size_t... Is>(std::index_sequence<Is...>) {
                ([&] {
                    using StageDescriptor = SystemStageDescriptor<Stage, System>;

                    const ComponentField<StageField>& stage = stageRegistry.getFieldOf<Stage>();

                    if (!stage) {
                        result = false;
                        return;
                    }
                    UpdateSystemDescriptor update = UpdateSystemDescriptor::of<Stage, System>();

                    if constexpr (IsActiveStage<Stage>) {
                        constexpr static auto dependencyCount = std::tuple_size_v<typename StageDescriptor::hard_deps>;
                        constexpr static auto sysReadsCount = std::tuple_size_v<typename StageDescriptor::resReads>;
                        constexpr static auto sysWritesCount = std::tuple_size_v<typename StageDescriptor::resWrites>;

                        if constexpr (dependencyCount > 0) {
                            const TypeUUID* dependencies = fillSystemStageDescriptor<typename StageDescriptor::hard_deps>();
                            update.systemDependencies = dependencies;
                            update.systemDependenciesCount = dependencyCount;
                        }

                        if constexpr (sysReadsCount > 0) {
                            const TypeUUID* reads = fillSystemStageDescriptor<typename StageDescriptor::resReads>();
                            update.systemReads = reads;
                            update.systemReadsCount = sysReadsCount;
                        }

                        if constexpr (sysWritesCount > 0) {
                            const TypeUUID* writes = fillSystemStageDescriptor<typename StageDescriptor::resWrites>();
                            update.systemWrites = writes;
                            update.systemWritesCount = sysWritesCount;
                        }
                    }

                    update.selfType = TypeUUID::of(kind, id);
                    new (mem + memIdx) RuntimeSystemStageDescriptor(stage.descriptor, update);
                    ++memIdx;
                }(), ...);
            });
            descriptor->stagesPtr = mem;
            descriptor->stageCount = memIdx;
            descriptor->id = id;

            return result;
        };

        if (cIndex.id() >= componentIndexToFieldIndex.size()) {
            componentIndexToFieldIndex.reserve(cIndex.id() + 1);

            while (componentIndexToFieldIndex.size() != componentIndexToFieldIndex.capacity()) {
                componentIndexToFieldIndex.emplace_back(0);
            }
            createSystem();
        } else if (systemFields[componentIndexToFieldIndex[cIndex.id()]].instance == nullptr) {
            createSystem();
        }
        return true;
    }

    template <typename System>
    TypeUUID findSystem() const {
        const auto cIndex = getComponentIndex<System>();

        if (componentIndexToFieldIndex.size() <= cIndex.id() || systemFields[componentIndexToFieldIndex[cIndex.id()]].instance == nullptr) {
            return TypeUUID::of(kind, 0);
        }
        return TypeUUID::of(kind, componentIndexToFieldIndex[cIndex.id()]);
    }

    template <typename System>
    const ComponentField<SystemField>& getFieldOf() {
        return systemFields[findSystem<System>().id()];
    }

    const ComponentField<SystemField>& getFieldOf(const TypeUUID type) const {
        return systemFields[type.id()];
    }

    template <typename System>
    TypeUUID isDeclared() {
        const auto cIndex = getComponentIndex<System>();

        if (componentIndexToFieldIndex.size() <= cIndex.id() || componentIndexToFieldIndex[cIndex.id()] == 0) {
            return TypeUUID::of(kind, 0);
        }
        return TypeUUID::of(kind, componentIndexToFieldIndex[cIndex.id()]);
    }

    template <typename System>
    bool has() const {
        return findSystem<System>().id() != 0;
    }

    bool has(const TypeUUID type) const {
        if (systemFields[type.id()].instance == nullptr) return false;
        return true;
    }

    ComponentKind getKind() const {
        return kind;
    }

    TypeUUID getTypeID(const ComponentIndex cIndex) const {
        return TypeUUID::of(kind, componentIndexToFieldIndex[cIndex.id()]);
    }
};