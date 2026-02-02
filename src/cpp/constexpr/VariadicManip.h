#pragma once
#include <tuple>

#include "PackManip.h"

namespace cexpr {
    namespace detail {
                template <
            typename Typelist, typename Result, size_t I,
            template <typename, typename> typename Predicate,
            template <typename, typename, size_t> typename OnPass,
            template <typename, typename, size_t> typename OnFail,
            typename
        >
        struct tuple_builder;

        template <
            typename Typelist, typename Result, size_t I,
            template <typename, typename> typename Predicate,
            template <typename, typename, size_t> typename OnPass,
            template <typename, typename, size_t> typename OnFail,
            bool DidPass,
            typename = void
        >
        struct false_branch {
            using value = std::type_identity<int>;
        };

        template <
            typename Typelist, typename Result, size_t I,
            template <typename, typename> typename Predicate,
            template <typename, typename, size_t> typename OnPass,
            template <typename, typename, size_t> typename OnFail,
            bool DidPass,
            typename = void
        >
        struct true_branch {
            using value = std::type_identity<int>;
        };

        template <
            typename Typelist, typename Result, size_t I,
            template <typename, typename> typename Predicate,
            template <typename, typename, size_t> typename OnPass,
            template <typename, typename, size_t> typename OnFail,
            typename Enable = void
        >
        struct tuple_builder {
            using element = std::tuple_element_t<I, Typelist>;
            using result = typename decltype([]{
                constexpr bool pred = Predicate<element, Result>::value;
                if constexpr (pred) {
                    return typename true_branch<Typelist, Result, I, Predicate, OnPass, OnFail, pred>::value{};
                } else {
                    return typename false_branch<Typelist, Result, I, Predicate, OnPass, OnFail, !pred>::value{};
                }
            }())::type;
        };

        template <
            typename Typelist, typename Result, size_t I,
            template <typename, typename> typename Predicate,
            template <typename, typename, size_t> typename OnPass,
            template <typename, typename, size_t> typename OnFail,
            bool DidPass
        >
        requires (DidPass == true)
        struct false_branch<Typelist, Result, I, Predicate, OnPass, OnFail, DidPass> {
            using value = std::type_identity<
                typename tuple_builder<
                    Typelist, typename OnFail<Typelist, Result, I>::result, I + 1, Predicate, OnPass, OnFail>::result
            >;
        };

        template <
            typename Typelist, typename Result, size_t I,
            template <typename, typename> typename Predicate,
            template <typename, typename, size_t> typename OnPass,
            template <typename, typename, size_t> typename OnFail,
            bool DidPass
        >
        requires (DidPass == true)
        struct true_branch<Typelist, Result, I, Predicate, OnPass, OnFail, DidPass> {
            using value = std::type_identity<
                typename tuple_builder<
                    Typelist, typename OnPass<Typelist, Result, I>::result, I + 1, Predicate, OnPass, OnFail>::result
            >;
        };

        template <
            typename Typelist, typename Result, size_t I,
            template <typename, typename> typename Predicate ,
            template <typename, typename, size_t> typename OnPass,
            template <typename, typename, size_t> typename OnFail
        >
        struct tuple_builder<Typelist, Result, I, Predicate, OnPass, OnFail, std::enable_if_t<(I >= std::tuple_size_v<Typelist>)>> {
            using result = Result;
        };
    }

    template <template <typename> typename Predicate>
    struct predicate {
        template <typename Element, typename Result>
        struct op {
            static constexpr bool value = Predicate<Element>::value;
        };
    };

    template <
        typename Typelist,
        template <typename...> typename Predicate      /* typename struct<Element, Result>::value -> boolean || struct<Element>::value */,
        template <typename, typename, size_t> typename OnPass /* typename struct<Typelist, Result, I>::result       */,
        template <typename, typename, size_t> typename OnFail /* typename struct<Typelist, Result, I>::result       */
    >
    using filter_typenames_t = typename detail::tuple_builder<Typelist, std::tuple<>, 0, Predicate, OnPass, OnFail>::result;

    template <typename Typelist, typename Result, size_t I>
    struct append_typename {
        using result = add_typename_to_tuple_t<std::tuple_element_t<I, Typelist>, Result>;
    };

    template <typename Typelist, typename Result, size_t I>
    struct append_decayed_typename {
        using result = add_typename_to_tuple_t<std::decay_t<std::tuple_element_t<I, Typelist>>, Result>;
    };

    template <typename List>
    struct not_in {
        template <typename Typelist, typename Result, size_t I>
        struct operation {
            using Type = std::tuple_element_t<I, Typelist>;
            static constexpr bool value = is_typename_in_tuple_v<Type, List>;
        };
    };

    template <template <typename> typename Pred>
    struct opposite {
        template <typename Element, typename Result>
        struct op {
            static constexpr bool value = not Pred<Element>::value;
        };
    };

    template <typename T, typename Result, size_t I>
    struct noop {
        using result = Result;
    };

    template <typename Typelist, typename Result, size_t I>
    struct add_typename_to_tuple {
        using result = add_typename_to_tuple_t<std::tuple_element_t<I, Typelist>, Result>;
    };

    template <typename T>
    using remove_duplicates = filter_typenames_t<T, is_typename_in_tuple, noop, add_typename_to_tuple>;
}
