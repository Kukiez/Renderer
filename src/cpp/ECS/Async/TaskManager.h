#pragma once

#include "Task.h"
#include <ECS/Global/Allocator.h>

struct TaskManager {
    ThreadPool* threadPool;
    ThreadAllocator* taskAllocator;

    template <typename Handle>
    void resume(Handle handle) {
        threadPool->enqueue([=]{
            handle.resume();
        });
    }

    template <typename T>
    void setTaskCompleted(Task<T>* task) {
        auto state = task->state();

        if (state == TaskState::HANDLE_DROPPED) {
            destroyTask(task);
        }
    }

    template <typename T>
    void destroyTask(Task<T>* task) {
        auto header = task->getLambdaHeader();
        header.type.destroy(header.inst);
        taskAllocator->deallocate(header.inst, header.type->size);

        task->~Task<T>();
        taskAllocator->deallocate(task, sizeof(Task<T>));
    }
};