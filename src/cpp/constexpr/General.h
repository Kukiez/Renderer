#pragma once
#include <iterator>

namespace cexpr {
    namespace detail {
        template <bool Condition, typename True, typename False>
    struct conditional_false_branch {
            using type = void;
        };

        template <bool Condition, typename True, typename False>
        requires (Condition == true)
        struct conditional_false_branch<Condition, True, False> {
            using type = False;
        };

        template <bool Condition, typename True, typename False>
        struct conditional_true_branch {
            using type = void;
        };

        template <bool Condition, typename True, typename False>
        requires (Condition == true)
        struct conditional_true_branch<Condition, True, False> {
            using type = True;
        };
    }

    template <typename T>
    struct type_identity {
        using type = T;
    };

    template <bool Condition, typename True, typename False>
    struct conditional {
        using type = decltype([] {
            if constexpr (Condition) {
                return type_identity<typename detail::conditional_true_branch<Condition, True, False>::type>{};
            } else {
                return type_identity<typename detail::conditional_false_branch<!Condition, True, False>::type>{};
            }
        }())::type;
    };

    template <bool Condition, typename True, typename False>
    using conditional_t = typename conditional<Condition, True, False>::type;

    template <typename Arr>
    constexpr auto bubble_sort_array(Arr&& arr) {
        for (size_t i = 0; i < arr.size(); ++i) {
            for (size_t j = 0; j < arr.size() - i - 1; ++j) {
                if (arr[j] > arr[j + 1]) {
                    typename std::decay_t<Arr>::value_type temp = arr[j];
                    arr[j] = arr[j + 1];
                    arr[j + 1] = temp;
                }
            }
        }
        return arr;
    }

    template <typename...>
    struct always_true {
        static constexpr bool value = true;
    };

    template <typename T>
    concept is_iteratable_v = requires(T t) {
            t.begin();
            t.end();
    };

    template <typename Range>
    using iterator_yield_t = decltype(*std::begin(std::declval<Range>()));

    template <typename Range, typename Yield>
    concept is_iteratable_with_yield_t = is_iteratable_v<Range> && std::is_same_v<iterator_yield_t<Range>, Yield>;

    struct true_type {
        static constexpr bool value = true;
    };

    struct false_type {
        static constexpr bool value = false;
    };

    template <size_t N>
    constexpr size_t strlen(const char (&str)[N]) {
        return N - 1;
    }
}
