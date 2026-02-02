#pragma once


struct TaskManager;
struct AwaitableManager;

struct Async {
    TaskManager* taskManager;
    AwaitableManager* awaitableManager;
};