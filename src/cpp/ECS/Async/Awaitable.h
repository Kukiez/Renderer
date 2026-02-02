#pragma once
#include <coroutine>
#include <ECS/Time.h>
#include "Async.h"
#include "AwaitableManager.h"

class Wait {
    friend struct AsyncScheduler;

    Async async;
    std::coroutine_handle<> coro;
    Time time{};
public:

    Wait(Async async, const Time duration) : async(async), time(Time::now() + duration) {}

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) noexcept {
        coro = handle;

        async.awaitableManager->await(*this);
    }

    void await_resume() const noexcept {}

    Wait operator co_await () {
        return *this;
    }

    bool operator < (const Wait& other) const {
        return time < other.time;
    }

    bool operator == (const Wait& other) const {
        return time == other.time;
    }

    bool operator != (const Wait& other) const {
        return time != other.time;
    }
};