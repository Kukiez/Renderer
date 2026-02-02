#pragma once
#include <tuple>
#include <type_traits>

namespace cexpr {
        namespace detail {
        template <size_t I = 0, typename T, typename Tuple>
        consteval size_t count_typename_appearances(size_t app = 0) {
            if constexpr (I == std::tuple_size_v<Tuple>) {
                return app;
            } else {
                if constexpr (std::is_same_v<T, std::tuple_element_t<I, Tuple>>) {
                    ++app;
                }
                return count_typename_appearances<I + 1, T, Tuple>(app);
            }
        }

        template <template <typename...> typename Base, typename Derived>
        struct is_base_of_template {
            template <typename... Args>
            consteval static auto test(Base<Args...>*) {
                return std::true_type{};
            }

            consteval static auto test(...) {
                return std::false_type{};
            }

            using value = decltype(test(std::declval<Derived*>()));
        };

        template <template <typename...> typename Template, typename Typename>
        struct is_instantiation_of {
            template <typename... Args>
            consteval static auto test(Template<Args...>) {
                return std::true_type{};
            }

            consteval static auto test(...) {
                return std::false_type{};
            }

            using value = decltype(test(std::declval<Typename>()));
        };

        template <typename T>
        struct instantiation {
            using types = void;
        };

        template <template <typename...> typename Template, typename... Args>
        struct instantiation<Template<Args...>> {
            using types = std::tuple<Args...>;
        };

        template <typename T>
        using instantiation_types_of_typename = typename instantiation<T>::types;

        template <template <typename...> typename Template, typename Typename>
        struct instantiation_typenames_of {
            using types = instantiation_types_of_typename<Typename>;
        };
    }

    template <typename T>
    concept is_template = requires {
        typename detail::instantiation<T>::types;
    };

    template <template <typename...> typename Base, typename T>
    concept is_base_of_template = detail::is_base_of_template<Base, T>::value::value;

    template <template <typename...> typename Template, typename Typename>
    constexpr static bool is_instantiation_of_v = detail::is_instantiation_of<Template, Typename>::value::value;

    template <template <typename...> typename Template, typename Typename>
    using template_typenames_of = typename detail::instantiation_typenames_of<Template, Typename>::types;

    template <typename Typename>
    using template_typenames_of_t = typename detail::instantiation<Typename>::types;

    template <is_template T>
    static constexpr auto template_typename_count_of = std::tuple_size_v<detail::instantiation<T>>;

    template <template <typename...> typename Template, typename T>
    concept is_template_of = is_base_of_template<Template, T>;

    template <template <typename...> typename A, template <typename...> typename B>
    struct is_same_template : std::false_type {};

    template <template <typename...> typename A>
    struct is_same_template<A, A> : std::true_type {};

    template <template <typename...> typename A, template <typename...> typename B>
    concept is_same_template_v = is_same_template<A, B>::value;


    template <typename Typename>
    using typenames_of_template_t = typename detail::instantiation<Typename>::types;

    template <typename Typename, is_template T>
    constexpr static auto typename_appearances_in_template_v = detail::count_typename_appearances<0, Typename, typenames_of_template_t<T>>();

    template <typename Typename, typename... Typenames>
    constexpr static auto typenames_appearances_v = detail::count_typename_appearances<0, Typename, std::tuple<Typenames...>>();
}
