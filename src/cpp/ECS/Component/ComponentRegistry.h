#pragma once
#include <memory/type_info.h>
#include "Component.h"
#include <tbb/concurrent_vector.h>

#include "TypeUUID.h"
#include "memory/byte_arena.h"
#include "memory/Span.h"

#include "ComponentTypeRegistry.h"

template <typename Field>
class ComponentKindRegistry {
    struct NewComponentField {
        ComponentField<Field> field;
        ComponentIndex index;
        TypeUUID type{};
    };
public:
    template <typename T, typename Derived>
    ComponentField<Field> onCreateField(this Derived& self, ComponentIndex cIndex) {
        return ComponentField<Field>::template of<T>();
    }
private:
    bool findType(ComponentIndex cIndex, TypeUUID& type) {
        for (auto& pendingField : newFields) {
            if (pendingField.index == cIndex) {
                type = pendingField.type;
                return true;
            }
        }
        const int newFieldIndex = static_cast<int>(cIndex.id() - fields.size());

        if (newFieldIndex >= newFields.size()) {
            return false;
        }
        type = newFields[newFieldIndex].type;
        return true;
    }

    auto& findField(this auto&& self, TypeUUID type) {
        const auto typeIndex = type.id();

        if (typeIndex < self.fields.size()) {
            return self.fields[typeIndex];
        }
        return self.newFields[typeIndex - self.fields.size()].field;
    }

    std::vector<uint16_t> componentIndexToFieldIndex;
    std::vector<ComponentField<Field>> fields;

    tbb::concurrent_vector<NewComponentField> newFields;
    ComponentKind myKind{};
public:
    explicit ComponentKindRegistry(const ComponentKind kind): myKind(kind) {
        fields.emplace_back();
        componentIndexToFieldIndex.emplace_back(0);
    }

    TypeUUID null() const {
        return TypeUUID::of(myKind, 0);
    }

    void flushNewFields() {
        if (newFields.empty()) {
            return;
        }
        unsigned highest = 0;

        for (auto& field : newFields) {
            highest = std::max(highest, field.index.id());
        }

        componentIndexToFieldIndex.reserve(highest + 1);

        while (componentIndexToFieldIndex.size() != componentIndexToFieldIndex.capacity()) {
            componentIndexToFieldIndex.emplace_back(0);
        }

        fields.reserve(fields.size() + newFields.size());
        for (auto& [field, index, type] : newFields) {
            fields.emplace_back(field);
            componentIndexToFieldIndex[index.id()] = type.id();

            if (fields.size() - 1 != type.id()) {
                assert(false);
            }
        }
        newFields.clear();
    }

    auto * getFieldOf(this auto&& self, const TypeUUID type) {
        return &self.findField(type);
    }

    const mem::type_info * getTypeInfoOf(const TypeUUID type) const {
        return findField(type).type;
    }

    template <typename T, typename Derived>
    TypeUUID getTypeID(this Derived& self) {
        using Type = std::decay_t<T>;

        assert(ComponentKind::of<Type>() == self.myKind);

        const auto index = ComponentTypeRegistry::getOrCreateComponentIndex<Type>(self.myKind);
        return self.getTypeID(index, self.template onCreateField<T>(index));
    }

    template <typename Derived>
    TypeUUID getTypeID(this Derived& self, const ComponentIndex index, ComponentField<Field>&& field) {
        if (index.id() >= self.componentIndexToFieldIndex.size()) {
            if (TypeUUID result; self.findType(index, result)) {
                return result;
            }
            const int id = static_cast<int>(self.fields.size() + self.newFields.size());
            self.newFields.emplace_back(std::move(field), index, TypeUUID::of(self.myKind, id));

            return TypeUUID::of(self.myKind, id);
        }
        return TypeUUID::of(self.myKind, self.componentIndexToFieldIndex[index.id()]);
    }

    template <typename Derived>
    TypeUUID getTypeID(this Derived& self, const ComponentIndex cIndex) {
        if (cIndex.id() >= self.componentIndexToFieldIndex.size()) {
            if (TypeUUID result; self.findType(cIndex, result)) {
                return result;
            }
            return TypeUUID::of(self.myKind, 0);
        }
        return TypeUUID::of(self.myKind, self.componentIndexToFieldIndex[cIndex.id()]);
    }

    bool hasField(this const auto& self, const ComponentIndex cIndex) {
        if (self.componentIndexToFieldIndex.size() <= cIndex.id()) {
            return false;
        }
        return self.componentIndexToFieldIndex[cIndex.id()] != 0;
    }

    size_t count() const {
        return fields.size() + newFields.size();
    }

    ComponentKind kind() const {
        return myKind;
    }

    auto& getFields() const {
        return fields;
    }
};

template <typename Registry>
concept IsComponentKindRegistry = cexpr::is_base_of_template<ComponentKindRegistry, Registry>;

ECSAPI std::ostream& operator << (std::ostream& os, const TypeUUID type);