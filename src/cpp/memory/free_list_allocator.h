#pragma once
#include "byte_arena.h"
#include <map>
#include "type_info.h"
#include "alloc.h"

namespace mem {
    class piece_list {
        std::map<size_t, size_t> freeBlocks;
    public:
        struct piece {
            size_t begin{};
            size_t count{};

            operator bool() const {
                return count != 0;
            }
        };

        piece_list() = default;

        explicit piece_list(const size_t pieces) {
            freeBlocks.emplace(0, pieces);
        }

        piece find(const size_t count) {
            for (auto it = freeBlocks.begin(); it != freeBlocks.end(); ++it) {
                const size_t start = it->first;

                if (const size_t size = it->second; size >= count) {
                    const size_t newStart = start + count;
                    const size_t newSize = size - count;

                    if (newSize != 0) {
                        freeBlocks.emplace(newStart, newSize);
                    }
                    freeBlocks.erase(it);
                    return {start, count};
                }
            }
            return {};
        }

        void free(const piece piece) {
            size_t offset = piece.begin;
            size_t count = piece.count;

            const auto next = freeBlocks.lower_bound(offset);

            if (next != freeBlocks.end() && offset + count == next->first) {
                count += next->second;
                freeBlocks.erase(next);
            }
            const auto newNext = freeBlocks.lower_bound(offset);

            if (newNext != freeBlocks.begin()) {
                const auto prev = std::prev(newNext);

                if (prev->first + prev->second == offset) {
                    offset = prev->first;
                    count += prev->second;
                    freeBlocks.erase(prev);
                }
            }
            freeBlocks.emplace(offset, count);
        }

        void clear() {
            freeBlocks.clear();
        }

        size_t remaining() const {
            size_t remaining = 0;
            for (auto it = freeBlocks.begin(); it != freeBlocks.end(); ++it) {
                remaining += it->second;
            }
            return remaining;
        }
    };

    class free_list_allocator_traits {
    protected:
        char* memory = nullptr;
        piece_list freeBlocks;
        size_t capacity = 0;
        size_t alignment = 8;
    public:
        free_list_allocator_traits() = default;

        free_list_allocator_traits(char* memory, const size_t capacity, size_t alignment) : capacity(capacity), memory(memory), alignment(alignment) {
            freeBlocks.free({0, capacity});
        }

        free_list_allocator_traits(const free_list_allocator_traits&) = delete;
        free_list_allocator_traits& operator = (const free_list_allocator_traits&) = delete;

        free_list_allocator_traits(free_list_allocator_traits&& other) noexcept
        : memory(other.memory),  freeBlocks(std::move(other.freeBlocks)), alignment(other.alignment),
            capacity(other.capacity)
        {
            other.memory = nullptr;
            other.capacity = 0;
            other.alignment = 0;
        }

        free_list_allocator_traits& operator=(free_list_allocator_traits&& other) noexcept {
            if (this != &other) {
                memory = other.memory;
                freeBlocks = std::move(other.freeBlocks);
                capacity = other.capacity;
                alignment = other.alignment;
                other.memory = nullptr;
                other.capacity = 0;
                other.alignment = 0;
            }
            return *this;
        }

        char* allocate(const type_info* type, const size_t count) {
            const size_t bytesRequired = padding(count, alignment);
            auto result = freeBlocks.find(bytesRequired);
            return result.count ? memory + result.begin : nullptr;
        }

        bool deallocate(void* start, size_t count) {
            const char* ptr = (char*)start;

            if (ptr < memory || ptr >= memory + capacity) {
                return false;
            }
            size_t offset = ptr - memory;
            freeBlocks.free({offset, count});
            return true;
        }

        bool does_pointer_belong_here(void* start) const {
            const char* ptr = (char*)start;
            if (ptr < memory || ptr >= memory + capacity) {
                return false;
            }
            return true;
        }

        void reset() {
            freeBlocks.clear();
            freeBlocks.free({0, capacity});
        }

        size_t remaining() const {
            return freeBlocks.remaining();
        }

        size_t get_capacity() const {
            return capacity;
        }

        char* data() const {
            return memory;
        }
    };

    class free_list_allocator {
        char* allocate_memory(size_t capacity) {
            return (char*)operator new (capacity, std::align_val_t{alignment});
        }
        void deallocate_memory(void* memory) {
            operator delete(memory, std::align_val_t{alignment});
        }

        std::vector<free_list_allocator_traits> allocators;
        size_t alignment = 64;
    public:
        free_list_allocator() = default;

        template <typename T>
        explicit free_list_allocator(const T capacity, size_t alignment = 64) : alignment(alignment) {
            allocators.emplace_back(allocate_memory(capacity), capacity, alignment);
        }

        char* allocate(const type_info* type, const size_t count) {
            for (int i = (int)allocators.size() - 1; i != 0; --i) {
                if (auto result = allocators[i].allocate(type, count); result) {
                    return result;
                }
            }
            size_t newCapacity = allocators.empty() ? type->size * count : std::max(type->size * count, allocators.back().get_capacity() * 2);
            allocators.emplace_back(allocate_memory(newCapacity), newCapacity, alignment);
            return allocators.back().allocate(type, count);
        }

        template <typename T>
        T* allocate(const size_t count) {
            return reinterpret_cast<T*>(allocate(type_info::of<T>(), count));
        }

        bool deallocate(void* start, size_t count) {
            for (int i = (int)allocators.size() - 1; i != 0; --i) {
                if (allocators[i].deallocate(start, count)) {
                    return true;
                }
            }
            return false;
        }

        bool deallocate(const type_info* type, void* start, const size_t count) {
            return deallocate(start, count * type->size);
        }

        void destroy_adjacent() {
            while (allocators.size() > 1) {
                allocators.pop_back();
            }
        }

        void reset_shrink() {
            destroy_adjacent();
            if (!allocators.empty()) {
                allocators.front().reset();
            }
        }

        void reset_compact() {
            size_t totalCapacity = 0;

            if (allocators.size() == 1) {
                return;
            }

            for (auto& allocator : allocators) {
                totalCapacity += allocator.get_capacity();
            }
            allocators.clear();
            allocators.emplace_back(allocate_memory(totalCapacity), totalCapacity, alignment);
        }

        bool does_pointer_belong_here(void* start) const {
            for (auto& allocator : allocators) {
                if (allocator.does_pointer_belong_here(start)) {
                    return true;
                }
            }
            return false;
        }
    };

    template <typename T>
    struct free_list_adaptor : default_destroy<T>, default_construct<T> {
        free_list_allocator* allocator = nullptr;

        free_list_adaptor() = default;

        free_list_adaptor(free_list_allocator* allocator) : allocator(allocator) {}
        free_list_adaptor(const free_list_adaptor& other) : allocator(other.allocator) {}

        free_list_adaptor& operator=(const free_list_adaptor& other) {
            if (this != &other) {
                allocator = other.allocator;
            }
            return *this;
        }

        free_list_adaptor(free_list_adaptor&& other) noexcept : allocator(other.allocator) {
        }

        free_list_adaptor& operator=(free_list_adaptor&& other) noexcept {
            if (this != &other) {
                allocator = other.allocator;
            }
            return *this;
        }

        T* allocate(const size_t count) {
            return reinterpret_cast<T*>(allocator->allocate(type_info::of<T>(), count));
        }

        void deallocate(T* ptr, const size_t count) {
            allocator->deallocate(reinterpret_cast<char*>(ptr), count * sizeof(T));
        }

        T* shrink_alloc(T* ptr, size_t oldSize, size_t newSize) {
            allocator->deallocate(ptr + oldSize, (oldSize - newSize) * sizeof(T));
            return ptr;
        }
    };

    template <typename T>
    struct allocator_traits<free_list_adaptor<T>> : default_allocator_traits {
        using propagate_on_container_copy_assignment = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;
        using is_always_equal = std::false_type;
    };
}