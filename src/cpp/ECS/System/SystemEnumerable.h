#pragma once
#include <memory/TypeOps.h>
#include <memory/function.h>

#include "ECS/ECSAPI.h"

using EnumerableConstructCallback = mem::function<void(void*, size_t), mem::small_buffer_dynamic_allocator<40>>;

struct SystemEnumerable {
    void* enumerable = nullptr;
    const mem::type_info* type = mem::type_info_of<void>;
    size_t count = 0;
    EnumerableConstructCallback constructCallback{};

    SystemEnumerable() = default;

    template <typename T, typename ConstructFn>
    static SystemEnumerable of(ConstructFn&& cfn) {
        SystemEnumerable result;
        result.type = mem::type_info_of<T>;
        result.constructCallback = std::forward<ConstructFn>(cfn);
        return result;
    }

    SystemEnumerable(const SystemEnumerable&) = delete;
    SystemEnumerable& operator = (const SystemEnumerable&) = delete;

    ECSAPI SystemEnumerable(SystemEnumerable&& other) noexcept;

    ECSAPI SystemEnumerable& operator = (SystemEnumerable&& other) noexcept;

    ECSAPI void initialize(size_t newCount);

    ECSAPI void reinitialize(size_t newCount);

    ECSAPI ~SystemEnumerable();

    size_t size() const {
        return count;
    }

    void* back() const {
        return mem::offset(type, enumerable, count);
    }

    void* get(const size_t idx) {
        return mem::offset(type, enumerable, idx);
    }

    const void* get(const size_t idx) const {
        return mem::offset(type, enumerable, idx);
    }
};

template <typename Enumerable>
class SystemEnumerableView {
    SystemEnumerable* enumerable = nullptr;
public:
    SystemEnumerableView() = default;
    explicit SystemEnumerableView(SystemEnumerable* enumerable) : enumerable(enumerable) {}

    auto begin() {
        return enumerable ? static_cast<Enumerable*>(enumerable->get(0)) : nullptr;
    }

    auto end() {
        return enumerable ? static_cast<Enumerable*>(enumerable->back()) : nullptr;
    }

    Enumerable& operator [] (const size_t idx) {
        cexpr::require(idx < size());
        return *static_cast<Enumerable*>(enumerable->get(idx));
    }

    size_t size() const {
        return enumerable ? enumerable->size() : 0;
    }

    bool empty() const {
        return size() == 0;
    }

    operator bool() const {
        return enumerable != nullptr;
    }
};