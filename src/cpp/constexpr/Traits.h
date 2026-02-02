#pragma once

namespace cexpr {
    // namespace detail {
    //
    //     template <size_t I, typename T, size_t baseHash = std::numeric_limits<size_t>::max(), size_t foundIndex = 0>
    //     consteval size_t find_smallest_tuple_index() {
    //         if constexpr (I >= std::tuple_size_v<T>) {
    //             return foundIndex;
    //         } else {
    //             constexpr size_t current_hash = type_hash<std::tuple_element_t<I, T>>();
    //             if constexpr (current_hash < baseHash) {
    //                 return find_smallest_tuple_index<I + 1, T, current_hash, I>();
    //             } else {
    //                 return find_smallest_tuple_index<I + 1, T, baseHash, foundIndex>();
    //             }
    //         }
    //     }
    // }
    //
    // template <size_t I, typename T, typename Tuple>
    // consteval size_t getTupleIndex() {
    //     if constexpr (std::is_same_v<std::tuple_element_t<I, Tuple>, T>) {
    //         return I;
    //     } else if constexpr (I + 1 < std::tuple_size_v<Tuple>) {
    //         return getTupleIndex<I + 1, T, Tuple>();
    //     } else {
    //         static_assert(false);
    //         return 0;
    //     }
    }

    // namespace detail {
    //     template <typename T, typename Typelist>
    //     struct remove_if_matching {
    //         using type = decltype([]{
    //             constexpr size_t appearances = typename_appearances_in_template_v<T, Typelist>;
    //
    //             if constexpr (appearances == 1) {
    //                 return std::type_identity<Typelist>{};
    //             } else {
    //                 constexpr size_t idx = find_tuple_typename_index_v<T, Typelist>;
    //                 constexpr size_t toRemoveIdx = find_tuple_typename_index_v<T, Typelist, idx + 1>;
    //
    //                 using other = remove_tuple_index_t<toRemoveIdx, Typelist>;
    //
    //                 return std::type_identity<other>{};
    //             }
    //         }())::type;
    //     };
    //
    //     template <typename T, typename Typelist>
    //     struct remove_all_matching_except_first {
    //         using type = decltype([]{
    //             using result = typename remove_if_matching<T, Typelist>::type;
    //
    //             if constexpr (std::is_same_v<result, Typelist>) {
    //                 return std::type_identity<result>{};
    //             } else {
    //                 return std::type_identity<remove_all_matching_except_first<T, result>::type>{};
    //             }
    //         }())::type;
    //     };
    // }
//}
