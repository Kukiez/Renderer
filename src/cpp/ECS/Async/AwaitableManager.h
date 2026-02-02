#pragma once
#include <memory/vector.h>
#include <algorithm>

class Wait;

struct AwaitableManager {
    mem::vector<Wait> timers;

    template <typename Awaitable>
    void await(Awaitable awaitable) {
        if constexpr (std::is_same_v<Awaitable, Wait>) {
            const auto it = std::ranges::lower_bound(timers.begin(), timers.end(), awaitable, [](auto& a, auto& b){
                return b < a;
            });
            timers.emplace(it, awaitable);            
        }
    }
};