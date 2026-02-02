#pragma once
#include <tbb/concurrent_vector.h>
#include <vector>
#include "ComponentTypeRegistry.h"

namespace ecs {
    template <typename T>
    concept HasOnCreateDefaultMetadata = requires(T t) {
        t.onCreateDefaultMetadata();
    };

    template <typename T>
    concept HasOnFlush = requires (T t)
    {
        t.onFlush();
    };

    template <typename T>
    concept IsValidTypeContext = requires(T t)
    {
        typename T::TypeID;
        typename T::TypeMetadata;
        // t.onCreateMetadata<C>(TypeID);
    };

    template <IsValidTypeContext CType>
    class EmptyTypeContext : public CType {
        using TypeID = CType::TypeID;
    public:
        EmptyTypeContext() = default;

        template <typename T>
        TypeID registerType() {
            return TypeID(ComponentIndexValue<CType, T>::value);
        }

        template <typename T>
        TypeID getTypeID() {
            return registerType<T>();
        }

        template <typename T>
        TypeID of() {
            return registerType<T>();
        }
    };

    template <typename CType>
    class TypeContext : public CType {
        using TypeID = CType::TypeID;
        using TypeMetadata = CType::TypeMetadata;

        std::vector<TypeMetadata> types;
        tbb::concurrent_vector<std::pair<ComponentIndex, TypeMetadata>> newTypes;
    public:
        TypeContext() {
            if constexpr (HasOnCreateDefaultMetadata<CType>) {
                types.emplace_back(CType::onCreateDefaultMetadata());
            } else {
                types.emplace_back();
            }
        }

        template <typename T>
        TypeID registerType() {
            auto cIndex = ComponentIndexValue<CType, T>::value;

            if (types.size() <= cIndex.id()) {
                for (auto& [type, metadata] : newTypes) {
                    if (type == cIndex) {
                        return TypeID(type.id());
                    }
                }
                newTypes.emplace_back(cIndex, CType::template onCreateMetadata<T>(TypeID(cIndex.id())));
            }
            return TypeID(cIndex.id());
        }

        const TypeMetadata& getTypeMetadata(TypeID type) {
            if (types.size() <= type.id()) {
                for (auto& [nType, nMeta] : newTypes) {
                    if (nType.id() == type.id()) {
                        return nMeta;
                    }
                }
                assert(false);
                std::unreachable();
            } else {
                return types[type.id()];
            }
        }

        void flushNewTypes() {
            if (newTypes.empty()) return;

            types.resize(types.size() + newTypes.size());

            for (auto& [type, meta] : newTypes) {
                types[type.id()] = meta;
            }
            newTypes.clear();

            if constexpr (HasOnFlush<CType>) {
                CType::onFlush();
            }
        }

        template <typename T>
        TypeID getTypeID() {
            return registerType<T>();
        }

        template <typename T>
        TypeID of() {
            return registerType<T>();
        }

        template <typename T>
        const TypeMetadata& getTypeMetadata() {
            return getTypeMetadata(of<T>());
        }
    };
}