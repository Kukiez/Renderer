#pragma once
#include <utility>

namespace cexpr {
    template <typename Tuple, typename Lambda>
    constexpr auto for_each_typename_in_tuple(Lambda&& lam) {
        return [&]<size_t... Is>(std::index_sequence<Is...>) {
            return lam.template operator()<std::tuple_element_t<Is, Tuple>...>();
        }(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
    }

    template <typename... Tuples, typename Lambda>
    constexpr auto for_each_typename_in_tuples(Lambda&& lam) {
        ([&] {
            cexpr::for_each_typename_in_tuple<Tuples>([&]<typename... Ts>() {
                return lam.template operator()<Ts...>();
            });
        }() ,...);
    }

    template <typename Tuple, typename Lambda>
    constexpr auto for_each_value_in_tuple(Tuple&& tuple, Lambda&& lam) {
        return [&]<size_t... Is>(std::index_sequence<Is...>) {
            return lam(std::get<Is>(tuple)...);
        }(std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{});
    }

    template <size_t Count, typename Lambda>
    constexpr auto for_each_index_in(Lambda&& lam) {
        return [&]<size_t... Is>(std::index_sequence<Is...>) {
            return lam.template operator()<Is...>();
        }(std::make_index_sequence<Count>{});
    }

    template <typename Tuple, typename Lambda>
    constexpr auto for_each_typename_and_index_in_tuple(Lambda&& lam) {
        return [&]<size_t... Is>(std::index_sequence<Is...>) {
            return lam.template operator()<std::tuple_element_t<Is, Tuple>...>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
        }(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
    }
}
