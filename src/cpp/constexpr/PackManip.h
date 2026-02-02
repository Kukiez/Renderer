#pragma once
#include <tuple>
#include <utility>

#include "TypeInfo.h"

namespace cexpr {
    namespace detail {
        template <typename Tuple>
        struct add_ref_to_tuple;

        template <typename... Args>
        struct add_ref_to_tuple<std::tuple<Args...>> {
            using tuple = std::tuple<Args&...>;
        };

        template <typename Tuple>
        consteval auto sort_tuple() {
            constexpr auto arr = []<size_t... Is>(std::index_sequence<Is...>) {
                std::array<std::pair<size_t, size_t>, std::tuple_size_v<Tuple>> arr{
                    std::make_pair(
                        type_hash<std::tuple_element_t<Is, Tuple>>(),
                        Is
                    )...
                };
                arr = bubble_sort_array(arr);
                return arr;
            }(std::make_index_sequence<std::tuple_size_v<Tuple>>{});

            return [&arr]<size_t... Is>(std::index_sequence<Is...>) {
                return std::type_identity<std::tuple<std::tuple_element_t<arr[Is].second, Tuple>...>>{};
            }(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
        }

        template <size_t I, typename T, typename Tuple>
        consteval size_t getTupleIndexOrTupleSize() {
            if constexpr (std::is_same_v<std::tuple_element_t<I, Tuple>, T>) {
                return I;
            } else if constexpr (I + 1 < std::tuple_size_v<Tuple>) {
                return getTupleIndexOrTupleSize<I + 1, T, Tuple>();
            } else {
                return std::tuple_size_v<Tuple>;
            }
        }

        template <size_t I, typename Superset, typename Subset>
        consteval bool isSubset() {
            if constexpr (I == std::tuple_size_v<Subset>) {
                return true;
            } else {
                constexpr auto idx = getTupleIndexOrTupleSize<0, std::tuple_element_t<I, Subset>, Superset>();

                if constexpr (idx == std::tuple_size_v<Superset>) {
                    return false;
                } else {
                    return isSubset<I + 1, Superset, Subset>();
                }
            }
        }

        template <typename T, typename Tuple, size_t I = 0>
        consteval auto find_typename_in_tuple() {
            if constexpr (I == std::tuple_size_v<Tuple>) {
                return I;
            } else {
                if constexpr (std::is_same_v<T, std::tuple_element_t<I, Tuple>>) {
                    return I;
                } else {
                    return find_typename_in_tuple<T, Tuple, I + 1>();
                }
            }
        }
    }

    template <typename... Tuples>
    using tuple_join_t = decltype(std::tuple_cat(std::declval<Tuples>()...));

    template <typename... Ts>
    using sort_to_tuple_t = decltype(detail::sort_tuple<std::tuple<Ts...>>())::type;

    template <typename T>
    using sort_tuple_t = decltype(detail::sort_tuple<T>())::type;

    template <typename Tuple>
    using add_ref_to_tuple_t = typename detail::add_ref_to_tuple<Tuple>;

        template <typename Superset, typename Subset>
    constexpr bool is_tuple_subset_v = detail::isSubset<0, Superset, Subset>();

    template <typename Tuple>
    using decay_tuple_t = decltype([]<size_t... Is>(std::index_sequence<Is...>) {
        return std::type_identity<std::tuple<std::decay_t<std::tuple_element_t<Is, Tuple>>...>>{};
    }(std::make_index_sequence<std::tuple_size_v<Tuple>>{}))::type;

    template <typename T>
    constexpr bool is_const_lvalue_ref_v = std::is_lvalue_reference_v<T> &&
        std::is_const_v<std::remove_reference_t<T>>;

    template <typename T, typename Tuple>
    constexpr bool is_constructible_from_tuple_v = []<size_t... Is>(std::index_sequence<Is...>) {
        return std::constructible_from<T, std::tuple_element_t<Is, Tuple>...>;
    }(std::make_index_sequence<std::tuple_size_v<Tuple>>{});

    template <typename Typename, typename Tuple, size_t BaseIndex = 0>
    constexpr auto find_tuple_typename_index_v = detail::find_typename_in_tuple<Typename, Tuple, BaseIndex>();

    template <typename Typename, typename Tuple>
    concept is_typename_in_tuple_v = (find_tuple_typename_index_v<Typename, Tuple> != std::tuple_size_v<Tuple>);

    template <typename T, typename TList>
    struct is_typename_in_tuple {
        static constexpr bool value = is_typename_in_tuple_v<T, TList>;
    };

    template <typename Typename, typename Tuple>
    using add_typename_to_tuple_t = decltype([]<size_t... Is>(std::index_sequence<Is...>) {
        return std::type_identity<std::tuple<std::tuple_element_t<Is, Tuple>..., Typename>>{};
    }(std::make_index_sequence<std::tuple_size_v<Tuple>>{}))::type;

    template <typename Tuple, typename... Ts>
    using add_typenames_to_tuple_t = decltype([]<size_t... Is>(std::index_sequence<Is...>) {
        return std::type_identity<std::tuple<std::tuple_element_t<Is, Tuple>..., Ts...>>{};
    }(std::make_index_sequence<std::tuple_size_v<Tuple>>{}))::type;

    template <size_t Index, typename Tuple>
    using remove_tuple_index_t = decltype([]<size_t... Is>(std::index_sequence<Is...>) {
            return std::type_identity<std::tuple<std::tuple_element_t<(Is < Index) ? Is : Is + 1, Tuple>...>>{};
        }(std::make_index_sequence<std::tuple_size_v<Tuple> - 1>{}))::type;

    template <typename T, typename Tuple>
    using remove_first_tuple_typename_t = decltype([]{
        constexpr auto I = find_tuple_typename_index_v<T, Tuple>;

        if constexpr (I != std::tuple_size_v<Tuple>) {
            return std::type_identity<remove_tuple_index_t<I, Tuple>>{};
        } else {
            return std::type_identity<Tuple>{};
        }
    }())::type;
}
