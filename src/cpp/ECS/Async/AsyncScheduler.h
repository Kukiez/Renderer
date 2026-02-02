#pragma once
#include <memory/vector.h>
#include <ECS/ThreadPool.h>
#include <ECS/Global/Allocator.h>
#include "TaskManager.h"
#include "AwaitableManager.h"
#include <ECS/Time.h>
#include "Async.h"
#include "Awaitable.h"
#include "Task.h"

struct AsyncScheduler {
    ThreadPool threadPool;

    ThreadAllocator& taskAllocator;
    TaskManager taskManager;
    AwaitableManager awaitManager;

    Async async;

    Time begin = Time::now();
public:
    AsyncScheduler(ThreadAllocator& taskAlloc) 
    : threadPool(false), taskAllocator(taskAlloc), async(&taskManager, &awaitManager),
    taskManager(&threadPool, &taskAllocator)
    {}

    template <typename Callable, typename... Args>
    auto launch(Callable&& callable, Args&&... args) {
        using Task = cexpr::function_return_t<Callable>;
        using HandleType = TaskHandle<typename Task::type>;
        using CallableType = std::decay_t<Callable>;

        Task* coro = static_cast<Task*>(
            taskAllocator.allocate(mem::type_info_of<Task>, 1)
        );

        CallableType* fn = static_cast<CallableType *>(taskAllocator.allocate(mem::type_info_of<CallableType>, 1));

        new (fn) CallableType(std::forward<CallableType>(callable));

        using CoroArgs = cexpr::function_args_t<Callable>;

        if constexpr (std::tuple_size_v<CoroArgs> > 50 /* && std::is_same_v<std::decay_t<std::tuple_element_t<0, CoroArgs>>, Async>*/) {
            new (coro) Task(
                (*fn)(async, std::forward<Args>(args)...)
            );
        } else {
            new (coro) Task(
                (*fn)(std::forward<Args>(args)...)
            );
        }

        coro->setAsync(async);
        coro->setLambdaHeader(LambdaHeader(fn, mem::type_info_of<CallableType>));

        threadPool.enqueue([coro]{
            coro->resume();
        });

        return HandleType(coro);
    }

    void poll() {
        auto now = Time::now();
        auto& timers = awaitManager.timers;

        if ((now - begin) >= Time::seconds(15)) {
            begin = now;

            taskAllocator.deallocatePending();
        }
        
        while (!timers.empty() && timers.back().time <= now) {
            timers.back().coro.resume();
            timers.pop_back();
        }
    }
};