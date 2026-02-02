#pragma once
#include "ArchetypeUtils.h"
#include "Entity.h"
#include "Archetype.h"
#include "ECS/Component/Types/PrimaryKindRegistry.h"
#include "ECS/Component/Types/Types.h"

template <typename... Ts>
struct ArchetypeDataIterator {
    using types = std::tuple<Ts...>;
    using DecayedTypes = std::tuple<std::decay_t<Ts>...>;
    Entity** entities;
    Archetype::TypeIndex* typeIndices;
    DataIndex* sizes;
    std::array<size_t, sizeof...(Ts)> indicesToTypeIndices;

    ArchetypeDataIterator(PrimaryKindRegistry& registry, Archetype& archetype) {
        static_assert(sizeof...(Ts) != 0, "Ts cannot be zero lol");
        entities = archetype.getEntities();
        typeIndices = archetype.getTypeIndices();

        if constexpr (sizeof...(Ts) > 1) {
            cexpr::for_each_index_in<sizeof...(Ts)>([&]<size_t... Is>() {
                ([&] {
                    indicesToTypeIndices[Is] = &archetype.findTypeIndex(registry.getTypeID<Ts>()) - typeIndices;
                }(), ...);
            });
        } else {
            indicesToTypeIndices[0] = &archetype.findTypeIndex(registry.getTypeID<Ts>()...) - typeIndices;
        }
        sizes = archetype.getSizes();
    }

    template <typename Fn>
    // fn(array[Ts*], chunkIdx, dataIdx, entities**)
    void forEachImpl(Fn&& fn) {
        for (int i = 0; i < 10; ++i) {
            if (sizes[i] == 0) continue;

            std::array dataPointers = [&]<size_t... Is>(std::index_sequence<Is...>) {
                return std::array{
                    [&]{
                        auto& typeIndex = typeIndices[indicesToTypeIndices[Is]];
                        void* data = typeIndex.chunks[i].data();

                        if constexpr (IsTrackedComponent<Ts> && std::is_reference_v<Ts> && !std::is_const_v<Ts>) {
                            typeIndex.changes[i].set_range(0, sizes[i]);
                        }
                        return data;
                    }()...
                };
            }(std::make_index_sequence<sizeof...(Ts)>{});

            for (DataIndex s = 0; s < sizes[i]; ++s) {
                fn(dataPointers, i, s, entities);
            }
        }        
    }

    template <typename Changed, typename Fn>
    // fn(array[Ts*], chunkIdx, dataIdx, entities**)
    void forEachChangedImpl(Fn&& fn) {
        constexpr static auto Index = cexpr::find_tuple_typename_index_v<std::decay_t<Changed>, DecayedTypes>;

        Archetype::TypeIndex* changedTypeIndex;

        if constexpr (Index != std::tuple_size_v<types>) {
            changedTypeIndex = &typeIndices[indicesToTypeIndices[Index]];
        } else {
            static_assert(false, "Changed Component must be in the Lambda's Parameters");
        }

        for (int i = 0; i < 10; ++i) {
            if (sizes[i] == 0) continue;

            std::array dataPointers = [&]<size_t... Is>(std::index_sequence<Is...>) {
                return std::array{
                    [&]{
                        auto& typeIndex = typeIndices[indicesToTypeIndices[Is]];
                        void* data = typeIndex.chunks[i].data();
                        return data;
                    }()...
                };
            }(std::make_index_sequence<sizeof...(Ts)>{});

            for (auto bit : changedTypeIndex->changes[i]) {
                fn(dataPointers, i, bit, entities);

                cexpr::for_each_index_in<sizeof...(Ts)>([&]<size_t... Is>() {
                    ([&] {
                        if constexpr (IsTrackedComponent<Ts> && std::is_reference_v<Ts> && !std::is_const_v<Ts> && Is != Index) {
                            auto& typeIndex = typeIndices[indicesToTypeIndices[Is]];
                            typeIndex.changes[i].set(bit);
                        }
                    }(), ...);
                });
            }
        }
    }

    template <size_t I>
    static decltype(auto) forward(auto& array, const size_t index) {
        using Decayed = std::tuple_element_t<I, DecayedTypes>;
        using Type = std::tuple_element_t<I, types>;

        Decayed& decayed = static_cast<Decayed*>(array[I])[index];
        return static_cast<Type>(decayed);
    }

    template <typename Fn>
    void forEach(Fn&& fn) {
        forEachImpl([&](auto& dataPointers, size_t i, size_t s, Entity**) {
            [&]<size_t... Is>(std::index_sequence<Is...>) {
                fn(forward<Is>(dataPointers, s)...);
            }(std::make_index_sequence<sizeof...(Ts)>{});            
        });
    }


    template <typename Fn>
    void forEachEntity(Fn&& fn) {
        forEachImpl([&](auto& dataPointers, size_t i, size_t s, Entity** entities) {
            [&]<size_t... Is>(std::index_sequence<Is...>) {
                fn(entities[i][s], forward<Is>(dataPointers, s)...);
            }(std::make_index_sequence<sizeof...(Ts)>{});
        });
    }

    template <typename Changed, typename Fn>
    void forEachEntityChanged(Fn&& fn) {
        forEachChangedImpl<Changed>([&](auto& dataPointers, size_t i, size_t s, Entity** entities) {
            [&]<size_t... Is>(std::index_sequence<Is...>) {
                fn(entities[i][s], forward<Is>(dataPointers, s)...);
            }(std::make_index_sequence<sizeof...(Ts)>{});
        });
    }
};

struct MatchingArchetypesIterator {
    PrimaryKindRegistry* registry;
    ArchetypeIndex* indices;
    PrimaryArchetype* archetypes;
    size_t count;

    template <typename args, typename Fn>
    void forEachImpl(Fn&& fn) {
        constexpr auto argsSize = std::tuple_size_v<args>;

        for (size_t i = 0; i < count; ++i) {
            auto& [archetype, edges] = archetypes[indices[i]];

            cexpr::for_each_typename_in_tuple<args>([&]<typename... Args>(){
                auto types = registry->getSortedTypeRange<Args...>();

                size_t hint = 0;

                for (auto& type : types) {
                    if (auto* typeIndex = archetype.findType(type, hint)) {
                        hint = archetype.indexOf(typeIndex);
                    } else {
                        return;
                    }
                }
                auto it = ArchetypeDataIterator<Args...>(*registry, archetype);
                fn(it);
            });
        }
    }

    template <typename... Ts>
    size_t countEntities() {
        size_t result = 0;

        for (size_t i = 0; i < count; ++i) {
            auto& [archetype, edges] = archetypes[indices[i]];

            [&]{
                auto& types = registry->getSortedTypeRange<Ts...>();

                size_t hint = 0;

                for (auto& type : types) {
                    if (auto* typeIndex = archetype.findType(type, hint)) {
                        hint = archetype.indexOf(typeIndex);
                    } else {
                        return;
                    }
                }
                result += archetype.getResidingEntitiesCount();
            }();
        }
        return result;
    }

    template <typename Fn>
    void forEach(Fn&& fn) {
        using args = cexpr::function_args_t<Fn>;

        if constexpr (cexpr::is_typename_in_tuple_v<Entity, cexpr::decay_tuple_t<args>>) {
            using args_noentity = cexpr::remove_tuple_index_t<0, args>;

            forEachImpl<args_noentity>([&](auto& it){
                it.forEachEntity(fn);
            });
        } else {
            forEachImpl<args>([&](auto& it){
                it.forEach(fn);
            });
        }
    }

    template <typename Changed, typename Fn>
    void forEachChanged(Fn&& fn) {
        using args = cexpr::function_args_t<Fn>;
        using args_noentity = cexpr::remove_tuple_index_t<0, args>;

        forEachImpl<args_noentity>([&](auto& it){
            it.template forEachEntityChanged<Changed>(std::forward<Fn>(fn));
        });
    }

    template <typename Fn>
    void forEachRawIterator(Fn&& fn) {
        using args = cexpr::function_args_t<Fn>;

        using args_noentity = cexpr::remove_first_tuple_typename_t<const Entity&, args>;

        forEachImpl<args_noentity>(fn);
    }
};