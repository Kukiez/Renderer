#pragma once
#include "PackIteration.h"
#include "PackManip.h"

namespace cexpr {
    template <typename... Ts>
    constexpr auto pack_hash_v = detail::pack_hash<Ts...>();

    template <typename... Ts>
    constexpr auto pack_stable_hash_v = cexpr::for_each_typename_in_tuple<sort_to_tuple_t<Ts...>>([]<typename... Args>() {
        return cexpr::pack_hash_v<Args...>;
    });

}
