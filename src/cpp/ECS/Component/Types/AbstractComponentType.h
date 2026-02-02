#pragma once
#include "../ComponentKind.h"

struct LevelContext;

struct AbstractComponentType {
    using SynchronizeFn = void(*)(void*, LevelContext& level);
    using OnFrameEndFn = void(*)(void*, LevelContext& level);

    using DestructorFn = void(*)(void*);

    void* instance = nullptr;

    SynchronizeFn onSynchronize = nullptr;
    OnFrameEndFn onFrameEnd = nullptr;
    DestructorFn destructor = nullptr;

    ComponentKind kind{};

    AbstractComponentType() = default;

    template <typename T>
    static AbstractComponentType of() {
        const ComponentKind kind = T::Kind;

        AbstractComponentType registry;
        registry.kind = kind;

        registry.onSynchronize = [] -> SynchronizeFn {
            if constexpr (requires(T t, LevelContext& l) { t.onSynchronize(l); }) {
                return [](void* ptr, LevelContext& level) {
                    T* t = static_cast<T*>(ptr);

                    t->onSynchronize(level);
                };
            } else {
                return nullptr;
            }
        }();

        registry.onFrameEnd = [] -> SynchronizeFn {
            if constexpr (requires(T t, LevelContext& l) { t.onFrameEnd(l); }) {
                return [](void* ptr, LevelContext& level) {
                    T* t = static_cast<T*>(ptr);

                    t->onFrameEnd(level);
                };
            } else {
                return nullptr;
            }
        }();

        registry.destructor = [](void* ptr) {
            static_cast<T*>(ptr)->~T();
        };
        return registry;
    }
};