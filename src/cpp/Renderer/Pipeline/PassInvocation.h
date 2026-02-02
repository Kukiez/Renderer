#pragma once

class Frame;

enum class PassInvocationID : unsigned {};

template <typename P>
concept IsPassInvocation = true;

template <typename P>
concept HasOnExecuteInvocation = requires(P& pass, const Frame* frame, const PassInvocationID id)
{
    pass.onExecute(frame, id);
};

struct PassInvocation {
    using OnExecute = void(*)(void* inst, const Frame* frame, const PassInvocationID id);

    OnExecute onExecute{};

    template <typename T>
    static constexpr PassInvocation create() {
        PassInvocation i;
        i.onExecute = [](void* inst, const Frame* frame, const PassInvocationID id) {
            static_cast<T*>(inst)->onExecute(frame, id);
        };
        return i;
    }
};

template <typename P>
class IPassInvocation {
public:
    IPassInvocation() {
        static_assert(HasOnExecuteInvocation<P>, "Pass must have onExecute(Frame* frame, PassInvocationID id)");
    }
};
#include "GraphicsPassInvocation.h"