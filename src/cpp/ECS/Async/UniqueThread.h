#pragma once
#include <oneapi/tbb/task_arena.h>
#include <oneapi/tbb/task_group.h>

class UniqueThread {
    tbb::task_arena arena;
    std::atomic<int> tasks = 0;
public:
    UniqueThread() : arena(1, 1) {}

    template <typename L>
    void enqueue(L&& lambda) {
        ++tasks;
        arena.enqueue([this, lambda = std::forward<L>(lambda)] {
            lambda();
            --tasks;
        });
    }

    void wait() {
        while (tasks > 0) {
            std::this_thread::yield();
        }
    }
};
