#include "SystemEnumerable.h"

SystemEnumerable::SystemEnumerable(SystemEnumerable &&other) noexcept {
    enumerable = other.enumerable;
    type = other.type;
    constructCallback = std::move(other.constructCallback);
    count = other.count;

    other.enumerable = nullptr;
    other.type = nullptr;
    other.count = 0;
}

SystemEnumerable & SystemEnumerable::operator=(SystemEnumerable &&other) noexcept {
    if (this != &other) {
        enumerable = other.enumerable;
        type = other.type;
        constructCallback = std::move(other.constructCallback);
        count = other.count;

        other.enumerable = nullptr;
        other.type = nullptr;
        other.count = 0;
    }
    return *this;
}

void SystemEnumerable::initialize(const size_t newCount) {
    this->count = newCount;
    enumerable = mem::allocate(type, count);
    constructCallback(enumerable, count);
}

void SystemEnumerable::reinitialize(const size_t newCount) {
    cexpr::require(enumerable);
    void* newEnumerable = mem::allocate(type, newCount);

    type.move(newEnumerable, enumerable, count);
    constructCallback(type.index(newEnumerable, count), newCount - count);
    mem::deallocate(type, enumerable);

    enumerable = newEnumerable;
    count = newCount;
}

SystemEnumerable::~SystemEnumerable() {
    if (enumerable) {
        type.destroy(enumerable, count);
        mem::deallocate(type, enumerable);
    }
}
