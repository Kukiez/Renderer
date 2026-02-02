#pragma once
#include <ECS/Entity/EntityRegistry.h>
#include <ECS/System/SystemRegistry.h>
#include <ECS/Time.h>
#include <ECS/ThreadPool.h>
#include <ECS/Async/AsyncScheduler.h>
#include <ECS/Global/Allocator.h>
#include <ECS/Global/FrameAllocator.h>
#include "ThisFrame.h"

class ECS;
class Level;

struct LevelContext {
    std::string name;

    ThreadAllocator allocator;    
    FrameAllocator frameAllocator;

    EntityRegistry registry;
    SystemRegistry systemRegistry;
    AsyncScheduler asyncOps;
    ThreadPool threadPool;
    LevelFrame thisFrame;

    SteadyTime lastFrame = SteadyTime(0);

    explicit LevelContext(std::string name):
        name(std::move(name)),
        registry(5000, 500),
        asyncOps(allocator),
        thisFrame(frameAllocator)
    {}

    LevelContext(LevelContext&) = delete;
    LevelContext(LevelContext&&) = delete;
    LevelContext& operator = (LevelContext&) = delete;
    LevelContext& operator = (LevelContext&&) = delete;
};