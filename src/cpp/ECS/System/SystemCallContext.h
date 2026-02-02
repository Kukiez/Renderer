#pragma once
#include <memory/type_info.h>

class SystemKindRegistry;

class SystemInvokeParams {
    void* params = nullptr;
    const mem::type_info* type = nullptr;
public:
    template <typename T>
    static SystemInvokeParams of(const T* data) {
        SystemInvokeParams params;
        params.params = (void*)data;
        params.type = mem::type_info_of<T>;
        return params;
    }

    template <typename T>
    T& get() const {
        assert(params);

        if (!is<T>()) {
            std::cout << "Invalid SystemInvokeParams: Expected: " << mem::type_info_of<T>->name << ", Actual: " << type->name << std::endl;
            assert(false);
        }
        return *static_cast<T*>(params);
    }

    template <typename T>
    bool is() const {
        return type == mem::type_info_of<T>;
    }

    operator bool() const {
        return params != nullptr;
    }
};

class SystemCallContext {
    void* stage = nullptr;
    SystemKindRegistry* systemRegistry;
    SystemInvokeParams invokeParams;
public:
    SystemCallContext() = default;

    explicit SystemCallContext(SystemKindRegistry& systemRegistry) : systemRegistry(&systemRegistry){}

    SystemCallContext(SystemKindRegistry& systemRegistry, const SystemInvokeParams invokeParams) : systemRegistry(&systemRegistry), invokeParams(invokeParams) {}

    void setStage(void* stage) {
        this->stage = stage;
    }

    void setInvokeParams(const SystemInvokeParams invokeParams) {
        this->invokeParams = invokeParams;
    }

    void* getStage() const {
        return stage;
    }

    template <typename System>
    System& in(this auto&& self) {
        return *static_cast<System*>(self.systemRegistry->template getFieldOf<System>().instance);
    }

    SystemInvokeParams getInvokeParams() const {
        return invokeParams;
    }
};
