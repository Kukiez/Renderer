#pragma once

#include "alloc.h"

namespace mem {
    template <typename Signature, IsDynamicAllocator Alloc = small_buffer_dynamic_allocator<32, 64>>
    class function;

    template <typename Ret, typename... Args, IsDynamicAllocator Alloc>
    class function<Ret(Args...), Alloc> {
        using InvokeFn = Ret(*)(void*, Args...);

        template <typename LambdaType>
        static Ret invokeImpl(void* ptr, Args... args) {
            auto& inv = *static_cast<LambdaType*>(ptr);

            return inv(std::forward<Args>(args)...);
        }

        void destroy_existing() {
            allocator.destroy(type, func, 1);
            allocator.deallocate(type, func, 1);
        }

        InvokeFn invokeFn = nullptr;
        void* func = nullptr;
        const type_info* type = type_info_of<void>;
        Alloc allocator;
    public:
        function() = default;

        function(const function& other) 
        : allocator(copy_forward_allocator(other.allocator))
        {
            if (other.func) {
                func = allocator.allocate(other.type, 1);
                allocator.copy_construct(other.type, func, other.func, 1);     
                invokeFn = other.invokeFn;
                type = other.type;       
            }
        }

        function& operator = (const function& other) {
            if (this == &other) return *this;

            if (!other.func) {
                clear();
                copy_assign_allocator(allocator, other.allocator);
                return *this;
            }

            if constexpr (IsAlwaysEqualAllocator<Alloc> || !ShouldPropagateOnCopy<Alloc>) {
                if (func && type == other.type) {
                    allocator.destroy(type, func, 1);
                    allocator.copy_construct(type, func, other.func, 1);
                } else {
                    destroy_existing();

                    func = allocator.allocate(other.type, 1);
                    allocator.copy_construct(other.type, func, other.func, 1);
                }
            } else {
                destroy_existing();

                allocator = other.allocator;

                func = allocator.allocate(other.type, 1);
                allocator.copy_construct(other.type, func, other.func, 1);
            }
            invokeFn = other.invokeFn;
            type = other.type;
            return *this;
        }

        function(function&& other) noexcept
        : allocator(move_forward_allocator(other.allocator)) {
            if (other.func) {
                if constexpr (IsAlwaysEqualAllocator<Alloc> || ShouldPropagateOnMove<Alloc>) {
                    func = other.func;
                    invokeFn = other.invokeFn;
                    type = other.type;

                    other.func = nullptr;
                    other.invokeFn = nullptr;
                    other.type = type_info_of<void>;
                } else {
                    func = allocator.allocate(other.type, 1);
                    allocator.copy_construct(other.type, func, other.func, 1);

                    invokeFn = other.invokeFn;
                    type = other.type;
                }
            }
        }

        function& operator = (function&& other) noexcept {
            if (this == &other) return *this;

            if (func) {
                destroy_existing();
            }

            if constexpr (IsAlwaysEqualAllocator<Alloc> || ShouldPropagateOnMove<Alloc>) {
                if constexpr (ShouldPropagateOnMove<Alloc>) {
                    allocator = std::move(other.allocator);
                }
                func = other.func;
                invokeFn = other.invokeFn;
                type = other.type;

                other.func = nullptr;
                other.invokeFn = nullptr;
                other.type = type_info_of<void>;
            } else {
                if (other.func) {
                    func = allocator.allocate(other.type, 1);
                    allocator.move_construct(other.type, func, other.func, 1);
                    invokeFn = other.invokeFn;
                    type = other.type;
                } else {
                    func = nullptr;
                    invokeFn = nullptr;
                    type = type_info_of<void>;
                }
            }
            return *this;
        }

        ~function() {
            if (func) {
                destroy_existing();
            }
        }

        template <typename Lambda>
        requires std::is_invocable_r_v<Ret, Lambda, Args...>
        function& operator = (Lambda&& lambda) {
            using LambdaType = std::decay_t<Lambda>;
            static_assert(std::is_copy_constructible_v<LambdaType>, "mem::function requires Invocable to be copy constructible");
            static_assert(std::is_move_constructible_v<LambdaType>, "mem::function requires Invocable to be copy constructible");

            if (func) {
                destroy_existing();
            }
            type = type_info_of<LambdaType>;
            func = allocator.allocate(type, 1);

            allocator.construct<LambdaType>(type, func, std::forward<Lambda>(lambda));

            invokeFn = &invokeImpl<LambdaType>;
            return *this;
        }

        bool operator == (const std::nullptr_t&) const {
            return invokeFn == nullptr;
        }

        bool operator != (const std::nullptr_t&) const {
            return invokeFn != nullptr;
        }

        operator bool() const {
            return invokeFn != nullptr;
        }

        Ret operator () (Args... args) {
            cexpr::require(invokeFn);
            return invokeFn(func, std::forward<Args>(args)...);
        }

        void clear() {
            if (func) {
                destroy_existing();
                func = nullptr;
                type = type_info_of<void>;
                invokeFn = nullptr;
            }
        }
    };
}