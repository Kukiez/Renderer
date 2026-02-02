#pragma once
#include "LevelAPI.h"

template <typename... Accesses>
class BasicView : public ViewBase {
    using AccessesTuple = std::tuple<Accesses...>;

    template <typename T>
    using SelectConstness = decltype([] {
        using Type = std::decay_t<T>;
        if constexpr (cexpr::is_typename_in_tuple_v<Type, AccessesTuple>) {
            return std::type_identity<Type>{};
        } else if constexpr (cexpr::is_typename_in_tuple_v<const Type, std::tuple<Accesses...>>) {
            return std::type_identity<const Type>{};
        } else {
            NotDeclaredAsReadOrWrite<BasicView, T>();
        }
    }())::type;
public:
    using ViewBase::ViewBase;

    template <AreSameComponentType... Ts>
    auto get(const Entity& e) {
        return query<Ts...>().get(e);
    }

    template <AreSameComponentType... Ts>
    bool has(const Entity& e) {
        return query<Ts...>().has(e);
    }

    template <AreSameComponentType... Ts>
    auto query() const {
        using QueryType = QueryTypeOf<SelectConstness<Ts>...>;
        return QueryType(level());
    }

    template <typename T>
    SelectConstness<T>& get() const {
        return level().systemRegistry.getSystem<T>();
    }

    template <typename... Other>
    requires (!std::is_same_v<AccessesTuple, typename BasicView<Other...>::AccessesTuple>)
    operator BasicView<Other...>() {
        ([] {
            using AccessedType = SelectConstness<Other>;
            if constexpr (std::is_const_v<AccessedType> && !std::is_const_v<Other>) {
                static_assert(false, "BasicView -> BasicView cast failed because a Component loses const qualifiers");
            }
        }(), ...);
        return BasicView<Other...>(level());
    }
};

template <typename... Accesses>
using View = BasicView<Accesses...>;