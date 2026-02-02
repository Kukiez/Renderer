#pragma once
#include <tbb/tbb.h>
#include <constexpr/Traits.h>

class ThreadPool {
    tbb::task_arena arena;
    tbb::task_group group;
    bool __DEBUG_RUN_SYNC = true;
public:
    ThreadPool() = default;
    ThreadPool(const bool __DEBUG_RUN_SYNC) : __DEBUG_RUN_SYNC(__DEBUG_RUN_SYNC) {}

    ~ThreadPool() {
        group.wait();
    }

    template <typename L>
    void execute(L&& lambda) {
        lambda();
        return;
        arena.execute(std::forward<L>(lambda));
    }

    template <typename L, typename... Args>
    void enqueue(L&& l, Args&&... args) {
        if (__DEBUG_RUN_SYNC) {
            l(std::forward<Args>(args)...);
            return;
        }
        arena.execute([&]{
            group.run([
                lam = std::forward<L>(l),
                args = std::forward_as_tuple(std::forward<Args>(args)...)
            ] {
                cexpr::for_each_value_in_tuple(args, [&](auto&&... vals){
                    lam(std::forward<decltype(vals)>(vals)...);
                });
            });            
        });
    }
};