#pragma once
#include <unordered_map>

#include "constexpr/FunctionInfo.h"
#include "constexpr/Template.h"

namespace mem {
    class megabyte {
        double mb = 0;

    public:
        template <typename T>
        constexpr explicit megabyte(T&& megabytes) : mb(static_cast<double>(megabytes)) {}

        size_t bytes() {
            return static_cast<size_t>(mb * 1024 * 1024);
        }

        operator size_t() {
            return bytes();
        }
    };

    template <typename T>
    class uninitialized {
        void destroy() {
            reinterpret_cast<T*>(buf)->~T();
        }
        
        alignas(alignof(T)) char buf[sizeof(T)];
        bool alive = false;
    public:
        uninitialized() = default;

        template <typename TT>
        requires std::constructible_from<T, TT>
        uninitialized(TT&& val) : alive(true) {
            new (buf) T(std::forward<TT>(val));
        }

        uninitialized(const uninitialized& other) 
        requires (!std::is_trivially_copyable_v<T> && std::is_copy_constructible_v<T>) 
        : alive(other.alive) {
            if (alive) {
                new (buf) T(*reinterpret_cast<T*>(other.buf));                
            }
        }

        uninitialized& operator = (const uninitialized& other)
        requires (!std::is_trivially_copyable_v<T> && std::is_copy_constructible_v<T>) 
        {
            if (this != &other) {
                if (alive) {
                    destroy();
                }
                alive = other.alive;
                new (buf) T(*reinterpret_cast<T*>(other.buf));
            }
            return *this;
        }

        uninitialized(uninitialized&& other)
        noexcept requires (!std::is_trivially_move_constructible_v<T> && std::is_move_constructible_v<T>)
        : alive(other.alive)
        {
            if (alive) {
                new (buf) T(std::move(*reinterpret_cast<T*>(other.buf)));
                other.destroy();
            }
            other.alive = false;
        }

        uninitialized& operator = (uninitialized&& other)
        noexcept requires (!std::is_trivially_move_constructible_v<T> && std::is_move_constructible_v<T>)
        {
            if (this != &other) {
                if (alive) {
                    destroy();
                }
                alive = other.alive;

                if (alive) {
                    new (buf) T(std::move(*reinterpret_cast<T*>(other.buf)));
                    other.destroy();
                }
                other.alive = false;
            }
            return *this;
        }

        uninitialized() requires (!std::is_trivially_destructible_v<T>) {
            if (alive) {
                destroy();
            }
        }

        template <typename TT>
        uninitialized& operator = (TT&& val) {
            if constexpr (std::is_trivial_v<T>) {
                new (buf) T(val);
            } else {
                if (alive) {
                    destroy();
                }
                new (buf) T(std::forward<TT>(val));
            }
            alive = true;
            return *this;
        }
    };

    template <typename Map, typename EmplaceFn, typename T>
    requires cexpr::is_template_of<std::unordered_map, std::decay_t<Map>>
    auto& find_or_emplace(Map&& map, T&& value, EmplaceFn&& orEmplace) {
        const auto it = map.find(std::forward<T>(value));

        if (it == map.end()) {
            if constexpr (cexpr::is_function_v<std::decay_t<EmplaceFn>>) {
                using Ret = cexpr::function_return_t<std::decay_t<EmplaceFn>>;
                return map.emplace(std::piecewise_construct,
                    std::forward_as_tuple(std::forward<T>(value)),
                    std::forward_as_tuple(std::forward<EmplaceFn>(orEmplace)())
                ).first->second;
            } else {
                return map.emplace(std::piecewise_construct,
                    std::forward_as_tuple(std::forward<T>(value)),
                    std::forward_as_tuple(std::forward<EmplaceFn>(orEmplace))
                ).first->second;
            }
        }
        return it->second;
    }

    template <typename Map, typename OrFn, typename T>
    requires cexpr::is_template_of<std::unordered_map, std::decay_t<Map>>
    && std::is_same_v<cexpr::function_return_t<std::decay_t<OrFn>>, typename std::decay_t<Map>::mapped_type>
    auto find_or(Map&& map, T&& value, OrFn&& orReturn) {
        const auto it = map.find(std::forward<T>(value));
        if (it == map.end()) {
            return std::forward<OrFn>(orReturn)();
        }
        return it->second;
    }
}
