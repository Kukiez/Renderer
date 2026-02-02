#pragma once

#include <ECS/Component/ComponentRegistry.h>
#include "RuntimeSystemDescriptor.h"

enum class StageLocalIndex : unsigned {
    INVALID = std::numeric_limits<unsigned>::max()
};

struct StageField {
    void* instance = nullptr;
    const RuntimeStageDescriptor* descriptor = nullptr;

    template <typename Stage>
    static StageField of(Stage* stage, const RuntimeStageDescriptor* descriptor) {
        return { stage, descriptor };
    }

    template <typename Stage>
    static StageField of() {
        return { nullptr, nullptr };
    }

    operator bool() const {
        return instance != nullptr;
    }
};

struct StageComponentType : ComponentType<StageComponentType> {};

class StageKindRegistry {
    mem::byte_arena<mem::same_alloc_schema, 64> stageAllocator;
    mem::byte_arena<mem::same_alloc_schema, 64> stageDescriptorAllocator;

    std::vector<int> componentIndexToFieldIndex;
    std::vector<ComponentField<StageField>> stageFields;

    ComponentKind kind;
public:
    StageKindRegistry(ComponentKind kind) : kind(kind) {
        stageFields.emplace_back(ComponentField<StageField>::of<NullStage>(nullptr, RuntimeStageDescriptor::Null()));
    }

    template <typename Stage, typename... Args>
    ComponentField<StageField>& addStage(Args&&... args) {
        const auto index = ComponentTypeRegistry::getOrCreateComponentIndex<Stage>(kind);

        auto createStage = [&] {
            const int id = static_cast<int>(stageFields.size());

            Stage* stage = stageAllocator.allocate<Stage>(1);
            new (stage) Stage(std::forward<Args>(args)...);

            auto descriptor = stageDescriptorAllocator.allocate<RuntimeStageDescriptor>(1);
            new (descriptor) RuntimeStageDescriptor(RuntimeStageDescriptor::of<Stage>());

            descriptor->selfType = TypeUUID::of(kind, id);
            
            stageFields.emplace_back(
                ComponentField<StageField>::of<Stage>(stage, descriptor)
            );
            componentIndexToFieldIndex[index.id()] = id;
        };

        if (index.id() >= componentIndexToFieldIndex.size()) {
            componentIndexToFieldIndex.reserve(index.id() + 1);

            while (componentIndexToFieldIndex.size() != componentIndexToFieldIndex.capacity()) {
                componentIndexToFieldIndex.emplace_back(-1);
            }
            createStage();
        } else if (componentIndexToFieldIndex[index.id()] == -1) {
            createStage();
        }
        return stageFields.back();
    }

    template <typename Stage>
    TypeUUID findStage() const {
        const auto index = ComponentTypeRegistry::getOrCreateComponentIndex<Stage>(kind);

        if (componentIndexToFieldIndex.size() <= index.id() || componentIndexToFieldIndex[index.id()] == -1) {
            return TypeUUID::of(kind, 0);
        }
        return TypeUUID::of(kind, componentIndexToFieldIndex[index.id()]);
    }

    template <typename Stage>
    const ComponentField<StageField>& getFieldOf() const {
        return stageFields[findStage<Stage>().id()];
    }

    const ComponentField<StageField>& getNullField() const {
        return stageFields.front();
    }

    const ComponentField<StageField>& getFieldOf(const TypeUUID type) const {
        return stageFields[type.id()];
    }

    template <typename Stage>
    bool has() const {
        return findStage<Stage>().id() != 0;
    }

    ComponentKind getKind() const {
        return kind;
    }

    TypeUUID getTypeID(const ComponentIndex cIndex) const {
        return TypeUUID::of(kind, componentIndexToFieldIndex[cIndex.id()]);
    }
};