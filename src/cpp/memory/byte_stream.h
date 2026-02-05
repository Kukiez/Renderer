#pragma once
#include "alloc.h"
#include "TypeOps.h"

namespace mem {
    template <typename Allocator>
    struct allocator_storage : public Allocator {
        allocator_storage() = default;

        allocator_storage(Allocator&& allocator) : Allocator(std::move(allocator)) {}

        template <typename ...Args>
        allocator_storage(Args&&... args) : Allocator(std::forward<Args>(args)...) {}

        allocator_storage(allocator_storage&& other) noexcept = default;
        allocator_storage& operator=(allocator_storage&& other) noexcept = default;

        Allocator& get_allocator() { return *this; }
        const Allocator& get_allocator() const { return *this; }
    };

    template <typename Allocator>
    struct allocator_ref_storage {
        Allocator* allocator;

        allocator_ref_storage() = default;
        allocator_ref_storage(Allocator& allocator) : allocator(&allocator) {}

        Allocator& get_allocator() { return *allocator; }
        const Allocator& get_allocator() const { return *allocator; }
    };

    template <typename Allocator, bool IsOwning = true>
    class basic_any_vector;

    template <typename Allocator, bool IsOwning>
    using select_allocator_storage = std::conditional_t<IsOwning, allocator_storage<Allocator>, allocator_ref_storage<Allocator>>;

    struct basic_any_vector_owning_members {
        typeindex myType{};
        void* dataPtr = 0;
        size_t mySize = 0;
        size_t myCapacity = 0;

        basic_any_vector_owning_members() = default;
        basic_any_vector_owning_members(const typeindex type) : myType(type) {}

        basic_any_vector_owning_members(basic_any_vector_owning_members&& other) noexcept
        : myType(other.myType), dataPtr(other.dataPtr), mySize(other.mySize), myCapacity(other.myCapacity) {
            other.dataPtr = nullptr;
            other.mySize = 0;
            other.myCapacity = 0;
            other.myType = typeindex{};
        }

        basic_any_vector_owning_members& operator=(basic_any_vector_owning_members&& other) noexcept {
            if (this != &other) {
                myType = other.myType;
                dataPtr = other.dataPtr;
                mySize = other.mySize;
                myCapacity = other.myCapacity;

                other.dataPtr = nullptr;
                other.mySize = 0;
                other.myCapacity = 0;
                other.myType = typeindex{};
            }
            return *this;
        }
        
        typeindex& get_type() {
            return myType;
        }

        void*& get_data() {
            return dataPtr;
        }

        size_t& get_size() {
            return mySize;
        }

        size_t& get_capacity() {
            return myCapacity;
        }

        typeindex get_type() const {
            return myType;
        }

        const void* get_data() const {
            return dataPtr;
        }

        size_t get_size() const {
            return mySize;
        }

        size_t get_capacity() const {
            return myCapacity;
        }
    };

    struct basic_any_vector_non_owning_members {
        typeindex myType{};
        void** dataPtr;
        size_t* mySize;
        size_t* myCapacity;

        typeindex& get_type() {
            return myType;
        }

        void*& get_data() const {
            return *dataPtr;
        }

        size_t& get_size() const {
            return *mySize;
        }

        size_t& get_capacity() const {
            return *myCapacity;
        }
    };

    template <bool IsOwning>
    using select_vector_members = std::conditional_t<IsOwning, basic_any_vector_owning_members, basic_any_vector_non_owning_members>;
    
    template <typename Allocator, bool IsOwning>
    class basic_any_vector : select_allocator_storage<Allocator, IsOwning>, select_vector_members<IsOwning>
    {
        using AllocatorBase = select_allocator_storage<Allocator, IsOwning>;
        using Members = select_vector_members<IsOwning>;

        friend AllocatorBase;

        size_t get_new_capacity(const size_t required) {
            constexpr size_t growthFactor = 2;

            const size_t max = std::max(Members::get_capacity() * growthFactor, required);
            return round_to<2>(max);
        }

        void reallocate(const size_t newCapacity) {
            auto* oldPtr = Members::get_data();
            size_t oldCapacity = Members::get_capacity();

            Members::get_capacity() = newCapacity;
            Members::get_data() = get_allocator().allocate(*Members::get_type(), newCapacity);

            const auto newPtr = Members::get_data();

            if (!oldPtr) {
                return;
            }

            if (oldPtr == newPtr) {
                return;
            }

            move(*Members::get_type(), newPtr, oldPtr, Members::get_size());

            get_allocator().deallocate(*Members::get_type(), oldPtr, oldCapacity);
        }
        void* emplace_back_impl(void* data, const size_t elements, auto forwardFn) {
            if (Members::get_size() + elements > Members::get_capacity()) {
                const size_t cap = std::max(elements, Members::get_capacity() * 2);
                reallocate(cap);
            }
            void* emplaceIdx = offset(*Members::get_type(), Members::get_data(), Members::get_size()++);
            (Members::get_type()->*forwardFn)(emplaceIdx, data, elements);
            return emplaceIdx;
        }
    public:
        template <typename AllocArg>
        basic_any_vector(AllocArg&& allocator, const typeindex& type, const size_t reserve = 0) : AllocatorBase(std::forward<AllocArg>(allocator)), Members(type) {
           if (reserve) this->reserve(reserve);
        }

        basic_any_vector(Allocator& allocator, const typeindex& type, void*& data, size_t& size, size_t& capacity) requires (!IsOwning)
        : AllocatorBase(allocator), Members(type, &data, &size, &capacity) {}

        basic_any_vector(const basic_any_vector&) requires (IsOwning) = delete;
        basic_any_vector& operator=(const basic_any_vector&) requires (IsOwning) = delete;

        ~basic_any_vector() requires (IsOwning) {
            clear();

            if (Members::get_data()) get_allocator().deallocate(*Members::get_type(), Members::get_data(), Members::get_capacity());
        }

        basic_any_vector(const basic_any_vector& other) requires (!IsOwning) = default;
        basic_any_vector& operator = (const basic_any_vector& other) requires (!IsOwning) = default;

        basic_any_vector(basic_any_vector&& other) noexcept = default;
        basic_any_vector& operator = (basic_any_vector&& other) noexcept = default;

        ~basic_any_vector() requires (!IsOwning) = default;

        void* move_emplace_back(void* data, const size_t elements = 1) {
            return emplace_back_impl(data, elements, typeindex::move);
        }

        void* copy_emplace_back(const void* data, const size_t elements = 1) {
            return emplace_back_impl(const_cast<void*>(data), elements, typeindex::copy);
        }

        template <typename ForwardFn>
        requires std::is_invocable_r_v<void, ForwardFn, void* /* dst */>
        void* forward_emplace_back(ForwardFn&& forward, const size_t count) {
            if (Members::get_size() + count > Members::get_capacity()) {
                reallocate(get_new_capacity(Members::get_size() + count));
            }
            void* emplaceIdx = offset(*Members::get_type(), Members::get_data(), Members::get_size());
            forward(emplaceIdx);
            Members::get_size() += count;
            return emplaceIdx;
        }

        void* unconstructed_emplace_back(const size_t elements = 1) {
            if (Members::get_size() + elements > Members::get_capacity()) {
                reallocate(get_new_capacity(Members::get_size() + elements));
            }
            void* emplaceIdx = offset(*Members::get_type(), Members::get_data(), Members::get_size());
            Members::get_size() += elements;
            return emplaceIdx;
        }

        void* default_emplace_back(void(*defaultConstructor)(void* dst, size_t count), size_t count) {
            if (Members::get_size() + count > Members::get_capacity()) {
                reallocate(get_new_capacity(Members::get_size() + count));
            }
            void* emplaceIdx = offset(*Members::get_type(), Members::get_data(), Members::get_size());
            defaultConstructor(emplaceIdx, count);
            Members::get_size() += count;
            return emplaceIdx;
        }

        template <typename T, typename... Args>
        T& emplace_back(Args&&... args) {
            if (!Members::get_type().template is<T>()) {
                assert(false && "Type mismatch");
            }
            if (Members::get_size() + 1 > Members::get_capacity()) {
                reallocate(get_new_capacity(Members::get_size() + 1));
            }
            T* dataT = static_cast<T*>(Members::get_data());
            new (&dataT[Members::get_size()++]) T(std::forward<Args>(args)...);
            return dataT[Members::get_size() - 1];
        }

        void reserve(const size_t count) {
           if (Members::get_capacity() < count) {
               reallocate(get_new_capacity(count));
               Members::get_capacity() = count;
           }
       }

        size_t size() const { return Members::get_size(); }
        size_t capacity() const { return Members::get_capacity(); }

        Allocator& get_allocator() { return AllocatorBase::get_allocator(); }

        void clear() {
            if (Members::get_data()) {
                destroy_at(*Members::get_type(), Members::get_data(), Members::get_size());
                Members::get_size() = 0;
            }
        }

        template <typename T>
        T* data() { return static_cast<T*>(Members::get_data()); }

        template <typename T>
        auto as_range() {
            return make_range(data<T>(), data<T>() + size());
        }

        template <typename Alloc>
        friend basic_any_vector<Alloc, false> as_any_vector_view(Alloc& allocator, const typeindex type, void* data, size_t size, size_t capacity);
    };

    using any_vector = basic_any_vector<dynamic_allocator>;

    template <typename Allocator>
    basic_any_vector<Allocator, false> as_any_vector_view(Allocator& allocator, const typeindex type, void*& data, size_t& size, size_t& capacity) {
        return {allocator, type, data, size, capacity};
    }

    template <typename Allocator, typename T>
    basic_any_vector<Allocator, false> as_any_vector_view(Allocator& allocator, const typeindex type, T*& data, size_t& size, size_t& capacity) {
        return {allocator, type, reinterpret_cast<void*&>(data), size, capacity};
    }
}