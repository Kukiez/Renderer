#pragma once
#include "alloc.h"
#include "TypeOps.h"

namespace mem {
    template <IsDynamicAllocator Alloc = small_buffer_dynamic_allocator<8, 8>>
    class any {
        void destroy_existing() {
            allocator.destroy(type, dataPtr, 1);
            allocator.deallocate(type, dataPtr, 1);
        }

        template <typename T, typename... Args>
        T& construct(Args&&... args) {
            if (dataPtr) {
                allocator.destroy(type, dataPtr, 1);
            }

            if (type != type_info_of<T>) {
                if (!dataPtr || !fits_in_address_of_type(type, type_info_of<T>)) {
                    allocator.deallocate(type, dataPtr, 1);
                    dataPtr = allocator.allocate(type_info_of<T>, 1);
                }
                type = type_info_of<T>; 
            }
            allocator.construct<T>(type, dataPtr, std::forward<Args>(args)...);
            return *static_cast<T*>(dataPtr);
        }

        void* dataPtr = nullptr;
        const type_info* type = type_info_of<void>;
        Alloc allocator;
    public:
        using AllocTraits = allocator_traits<Alloc>;

        any() = default;

        template <typename T>
        any(T&& value) {
            emplace<std::decay_t<T>>(std::forward<T>(value));
        }

        template <typename TAlloc, typename T>
        any(TAlloc&& allocator, T&& value) : allocator(std::forward<TAlloc>(allocator)) {
            emplace<std::decay_t<T>>(std::forward<T>(value));
        }

        template <typename T, typename... Args>
        any(in_place_t<T>, Args&&... args) {
            emplace<T>(std::forward<Args>(args)...);
        }

        any(const any& other)
        : allocator(copy_forward_allocator(other.allocator)) {
            if (other.dataPtr) {
                dataPtr = allocator.allocate(other.type, 1);
                allocator.copy_construct(other.type, dataPtr, other.dataPtr, 1);
                type = other.type;
            }
        }

        any& operator=(const any& other) {
            if (this == &other) return *this;

            if (!other.dataPtr) {
                clear();
                copy_assign_allocator(allocator, other.allocator);
                return *this;
            }

            if constexpr (IsAlwaysEqualAllocator<Alloc> || !ShouldPropagateOnCopy<Alloc>) {
                if (dataPtr && type == other.type) {
                    allocator.destroy(type, dataPtr, 1);
                    allocator.copy_construct(type, dataPtr, other.dataPtr, 1);
                } else {
                    destroy_existing();
                    dataPtr = allocator.allocate(other.type, 1);
                    allocator.copy_construct(other.type, dataPtr, other.dataPtr, 1);
                }
            } else {
                destroy_existing();
                allocator = other.allocator;
                dataPtr = allocator.allocate(other.type, 1);
                allocator.copy_construct(other.type, dataPtr, other.dataPtr, 1);
            }

            type = other.type;
            return *this;
        }

        any(any&& other) noexcept
        : allocator(move_forward_allocator(other.allocator)) {
            if (other.dataPtr) {
                if constexpr (IsAlwaysEqualAllocator<Alloc> || ShouldPropagateOnMove<Alloc>) {
                    dataPtr = other.dataPtr;
                    type = other.type;

                    other.dataPtr = nullptr;
                    other.type = type_info_of<void>;
                } else {
                    dataPtr = allocator.allocate(other.type, 1);
                    allocator.move_construct(other.type, dataPtr, other.dataPtr, 1);
                    type = other.type;
                }
            }
        }

        any& operator=(any&& other) noexcept {
            if (this == &other) return *this;

            if (dataPtr) destroy_existing();

            if constexpr (IsAlwaysEqualAllocator<Alloc> || ShouldPropagateOnMove<Alloc>) {
                dataPtr = other.dataPtr;
                type = other.type;

                other.dataPtr = nullptr;
                other.type = type_info_of<void>;
            } else {
                if (other.dataPtr) {
                    dataPtr = allocator.allocate(other.type, 1);
                    allocator.move_construct(other.type, dataPtr, other.dataPtr, 1);
                    type = other.type;
                } else {
                    dataPtr = nullptr;
                    type = type_info_of<void>;
                }
            }

            return *this;
        }

        ~any() {
            if (dataPtr) {
                destroy_existing();
            }
        }

        template <typename TAlloc>
        requires std::constructible_from<Alloc, TAlloc>
        any(TAlloc&& alloc) : allocator(std::forward<TAlloc>(alloc)) {}

        template <typename T>
        any& operator = (T&& val) {
            construct<std::decay_t<T>>(std::forward<T>(val));
            return *this;
        }

        template <typename T, typename... Args>
        T& emplace(Args&&... args) {
            return construct<T>(std::forward<Args>(args)...);
        }

        template <typename T>
        T* get() {
            if (type_info_of<T> == type) {
                return static_cast<T*>(dataPtr);
            }
            return nullptr;
        }

        template <typename T>
        bool is() const {
            return type_info_of<T> == type;
        }

        template <typename T>
        const T* get() const {
            if (type_info_of<T> == type) {
                return static_cast<const T*>(dataPtr);
            }
            return nullptr;
        }

        template <typename T>
        T& get_or_default() {
            if (dataPtr) {
                return *static_cast<T*>(dataPtr);
            } else {
                return emplace<T>();
            }
        }

        void clear() {
            if (dataPtr) {
                destroy_existing();
                dataPtr = nullptr;
                type = type_info_of<void>;
            }
        }
    };

    template <typename TAlloc, typename TVal>
    auto make_any(TAlloc&& allocator, TVal&& value) {
        return mem::any<std::decay_t<TAlloc>>(std::forward<TAlloc>(allocator), std::forward<TVal>(value));
    }
}