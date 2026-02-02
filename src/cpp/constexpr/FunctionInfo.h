#pragma once
#include <tuple>

namespace cexpr {
    template <typename T>
    struct function_traits;

    template <typename R, typename... Args>
    struct function_traits<R(*)(Args...)> {
        using return_type = R;
        using args = std::tuple<Args...>;
    };

    template <typename R, typename C, typename... Args>
    struct function_traits<R(C::*)(Args...)> {
        using return_type = R;
        using args = std::tuple<Args...>;
    };

    template <typename R, typename C, typename... Args>
    struct function_traits<R(C::*)(Args...) const> {
        using return_type = R;
        using args = std::tuple<Args...>;
    };

    template <typename Callable>
    using function_args_t = decltype([] {
        if constexpr (requires { &Callable::operator(); }) {
            return function_traits<decltype(&std::decay_t<Callable>::operator())>{};
        } else {
            return function_traits<std::decay_t<Callable>>{};
        }
    }())::args;

    template <typename Callable>
    using function_return_t = decltype([] {
        if constexpr (requires { &Callable::operator(); }) {
            return function_traits<decltype(&std::decay_t<Callable>::operator())>{};
        } else {
            return function_traits<std::decay_t<Callable>>{};
        }
    }())::return_type;

    template <typename Callable>
    concept is_function_v = requires(Callable c)
    {
        typename decltype(&Callable::operator());
    };
}
