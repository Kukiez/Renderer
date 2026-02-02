#pragma once
#include "SystemKindRegistry.h"
#include "StageKindRegistry.h"

struct ExecutionGraphCreationContext {
    mem::byte_arena<> allocator = mem::byte_arena<>(0.1 * 1024 * 1024);

    SystemKindRegistry& systemKindRegistry;
    StageKindRegistry& stageKindRegistry;

    ExecutionGraphCreationContext(SystemKindRegistry& systemRegistry, StageKindRegistry& descRegistry)
        : systemKindRegistry(systemRegistry), stageKindRegistry(descRegistry) {}
};