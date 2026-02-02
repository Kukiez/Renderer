#pragma once
#include <coroutine>
#include "Async.h"

struct TaskID {
    static unsigned next() {
        static unsigned next = 0;
        return next++;
    }
};

enum class TaskState : uint8_t {
    PENDING = 0,
    COMPLETED = 1,
    HANDLE_DROPPED = 2,
    WAITED_ON = 4
};

struct LambdaHeader {
    void* inst;
    const mem::type_info* type;
};

template <typename T>
struct Task {
    using type = T;

    struct promise_type {
        using ParentTask = void*;

        alignas(alignof(T)) char result[sizeof(T)]; 
        unsigned id = TaskID::next();
        std::coroutine_handle<> continuation;
        std::atomic<TaskState> state = TaskState::PENDING;
        Async async;
        Task* task;
        LambdaHeader lambda;

        Task get_return_object() {
            return Task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }

        std::suspend_always initial_suspend() { 
            return {}; 
        }

        auto final_suspend() noexcept { 
            struct final_awaiter {
                bool await_ready() noexcept { return false; }

                void await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
                    auto& promise = handle.promise();
                    if (promise.continuation) {
                        promise.async.taskManager->resume(promise.continuation);
                    } else {
                        promise.async.taskManager->setTaskCompleted(promise.task);
                    }
                }

                void await_resume() noexcept {}
            };
            auto expected = TaskState::PENDING;
            state.compare_exchange_strong(expected, TaskState::COMPLETED);

            return final_awaiter{};
        }

        void return_value(T value) {
            new (result) T(std::move(value));
        }

        void unhandled_exception() {
        //    state.store(TaskState::EXCEPTION);
        }

        ~promise_type() {
            // if != EXCEPTION ?
            reinterpret_cast<T*>(result)->~T();
        }
    }; 

    struct awaiter {
        Task& task;

        awaiter(Task& task) : task(task) {}

        bool await_ready() noexcept {
            return task.ready();
        }

        template <typename Other>
        void await_suspend(std::coroutine_handle<Other> parent) noexcept {
            auto& promise = task.handle.promise();
            promise.continuation = parent;

            promise.async = parent.promise().async;

            promise.async.taskManager->resume(task.handle);
        }

        T await_resume() noexcept {
            return std::move(task.result());
        }
    };
private:
    std::coroutine_handle<promise_type> handle; 
public:
    Task(std::coroutine_handle<promise_type> handle) : handle(handle) {
        handle.promise().task = this;
    }

    Task(Task&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }

    Task& operator = (Task&& other) noexcept {
        if (this != &other) {
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }

    ~Task() {
        if (handle) {
            handle.destroy();
        }
    }

    bool ready() const {
        return !handle || handle.done();
    }

    void resume() {
        if (!ready()) {
            handle.resume();
        }
    }

    TaskState state() const {
        return handle.promise().state;
    }

    void setState(TaskState state) {
        handle.promise().state = state;
    }

    bool isCompleted() const {
        return handle.promise().state == TaskState::COMPLETED;
    }

    T get() {
        return std::move(result());
    }

    T& result() {
        return *reinterpret_cast<T*>(handle.promise().result);
    }

    operator T& () {
        return result();
    }

    auto operator co_await () {
        return awaiter{*this};
    }

    void setAsync(Async async) {
        handle.promise().async = async;
    }

    void setLambdaHeader(LambdaHeader header) {
        handle.promise().lambda = header;
    }

    LambdaHeader getLambdaHeader() {
        return handle.promise().lambda;
    }

    Async getAsync() {
        return handle.promise().async;
    }
};

template <> struct Task<void> : public Task<std::monostate> {};

template <typename T>
class TaskHandle {
    Task<T>* task;
    bool retrieved = false;
public:
    TaskHandle(Task<T>* task) : task(task) {}

    TaskHandle(const TaskHandle&) = delete;
    TaskHandle& operator = (const TaskHandle&) = delete;

    TaskHandle(TaskHandle&& other) noexcept : retrieved(other.retrieved), task(other.task) {
        other.retrieved = true;
        other.task = nullptr;
    }

    TaskHandle& operator = (TaskHandle&& other) noexcept {
        if (this != &other) {
            retrieved = other.retrieved;
            task = other.task;

            other.retrieved = true;
            other.task = nullptr;
        }
        return *this;
    }

    ~TaskHandle() {
        if (!retrieved) {
            task->setState(TaskState::HANDLE_DROPPED);
        }
    }

    bool ready() const {
        return !retrieved && task->isCompleted();
    }

    T get() {
        cexpr::require(!retrieved);

        if (task->isCompleted()) {
            T t = task->get();
            task->getAsync().taskManager->destroyTask(task);
            retrieved = true;
            return std::move(t);
        } else {
            task->setState(TaskState::WAITED_ON);
            assert(false);
        }
    }

    TaskState state() {
        return task->state();
    }
};