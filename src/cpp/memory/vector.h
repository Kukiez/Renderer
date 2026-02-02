#pragma once
#include <type_traits>
#include <constexpr/assert.h>
#include "alloc.h"
#include "Span.h"
#include "constexpr/Template.h"

namespace mem {
    template <typename vector_iterator1, typename vector_iterator2>
    concept are_vector_iterator_alike = std::is_same_v<typename vector_iterator1::decayed_value_type, typename vector_iterator2::decayed_value_type>;

    template <typename T>
    struct vector_iterator {
        using value_type = T;
        using decayed_value_type = std::decay_t<T>;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::random_access_iterator_tag;
    private:
        T* begin;
    public:
        vector_iterator() = default;
        vector_iterator(T* begin) : begin(begin) {}
        
        template <typename Other>
        requires are_vector_iterator_alike<vector_iterator, Other>
        bool operator == (const Other& other) const {
            return begin == other.begin;
        }

        template <typename Other>
        requires are_vector_iterator_alike<vector_iterator, Other>
        bool operator != (const Other& other) const {
            return begin != other.begin;
        }

        bool operator > (const vector_iterator& other) const {
            return begin > other.begin;
        }

        bool operator < (const vector_iterator& other) const {
            return begin < other.begin;
        }

        bool operator >= (const vector_iterator& other) const {
            return begin >= other.begin;
        }

        bool operator <= (const vector_iterator& other) const {
            return begin <= other.begin;
        }

        vector_iterator operator + (const difference_type count) const {
            return {
                begin + count
            };
        }

        friend vector_iterator operator + (const difference_type count, const vector_iterator& it) {
            return it + count;
        }

        vector_iterator& operator += (const difference_type count) {
            begin += count;
            return *this;
        }
        
        vector_iterator operator - (const difference_type count) const {
            return {
                begin - count
            };
        }

        difference_type operator - (const vector_iterator& other) const {
            return begin - other.begin;
        }

        vector_iterator& operator -= (const difference_type count) {
            begin -= count;
            return *this;
        }

        vector_iterator& operator ++() {
            ++begin;
            return *this;
        }

        vector_iterator operator ++(int) {
            auto temp = *this;
            ++begin;
            return temp;
        }

        vector_iterator& operator --() {
            --begin;
            return *this;
        }

        vector_iterator operator --(int) {
            auto tmp = *this;
            --begin;
            return tmp;
        }

        reference operator *() {
            return *begin;
        }
        
        const reference operator *() const {
            return *begin;
        }

        pointer operator->() { return begin; }
        const pointer operator->() const { return begin; }

        reference operator[](difference_type diff) { return begin[diff]; }
        const reference operator[](difference_type diff) const { return begin[diff]; }


        pointer data() {
            return begin;
        }

        const pointer data() const {
            return begin;
        }
    };

    template <typename T>
    struct vector_iterator_holder {
        T* first, *second;

        auto begin() {
            return vector_iterator<T>{first};
        }
        auto end() {
            return vector_iterator<T>{second};
        }
    };

    template <
        typename T,
        typename Alloc = default_allocator<T>,
        typename ReallocationSchema = doubling_schema,
        typename size_type = size_t
    > requires IsAllocator<T, Alloc>
    struct vector {
        using value_type = T;
        using alloc_traits = allocator_traits<Alloc>;
        using allocator_type = Alloc;
    private:
        template <typename... Args>
        void construct(T* otherPtr, const size_type index, Args&&... args) {
            allocator.construct(otherPtr + index, std::forward<Args>(args)...);
        }

        void destroy(const size_type index) {
            allocator.destroy(ptr + index);
        }

        void destroy_all() {
            for (size_type i = 0; i < current; ++i) {
                destroy(i);
            }
        }

        void deallocate() {
            if (ptr) {
                allocator.deallocate(ptr, max);
            }
        }

        vector_iterator<T> make_iterator(size_type idx = 0) {
            return {ptr + idx};
        }

        vector_iterator<const T> make_const_iterator(size_type idx = 0) {
            return {ptr + idx};
        }

        bool has_memory(const size_type required) {
            return current + required <= max;
        }

        void realloc(const size_type newCapacity) {
            T* newPtr = allocator.allocate(newCapacity);

            if (ptr != newPtr) {
                for (size_type i = 0; i < current; ++i) {
                    construct(newPtr, i, std::move(ptr[i]));
                }                    
                destroy_all();
                deallocate();
                ptr = newPtr;                
            }
            max = newCapacity;
        }

        void ensure_has_memory(const size_type required) {
            if (!has_memory(required)) {
                const size_type newCapacity = static_cast<size_type>(reallocSchema.grow(max, required));
                realloc(newCapacity);
            }
        }

        bool ensure_has_memory_reserve(const size_type required, const size_type index, const size_type len) {
            if (!has_memory(required)) [[unlikely]] {
                const size_type newCapacity = static_cast<size_type>(reallocSchema.grow(max, required));
                T* newPtr = allocator.allocate(newCapacity);

                if (ptr != newPtr) {
                    const size_type remaining = current - index;
                    const size_type secondPassOffset = index + len;

                    for (size_type i = 0; i < index; ++i) {
                        construct(newPtr, i, std::move(ptr[i]));
                    }
                    for (size_type i = 0; i < remaining; ++i) {
                        construct(newPtr, secondPassOffset + i, std::move(ptr[index + i]));
                    }
                    destroy_all();
                    deallocate();
                    ptr = newPtr;                    
                } else {
                    make_space(index, len);
                }
                max = newCapacity;
                return true;
            }
            return false;
        }

        template <typename ItBegin, typename ItEnd>
        void insert_inplace(const size_type index, const size_t length, ItBegin&& begin, ItEnd&& end) {
            for (size_type i = current; i > index; --i) {
                construct(ptr, i + length, std::move(ptr[i - 1]));
                destroy(i - 1);
            }
            size_type count = 0;
            for (auto it = begin; it != end; ++it) {
                construct(ptr, index + count, *it);
                ++count;
            }
            current += length;
        }

        template <typename ItBegin, typename ItEnd>
        void realloc_insert(const size_type index, const size_t length, ItBegin&& begin, ItEnd&& end) {
            const size_type newCapacity = static_cast<size_type>(reallocSchema.grow(max, current + length));

            T* newPtr = allocator.allocate(newCapacity);
            if (ptr != newPtr) {
                for (size_type i = 0; i < index; ++i) {
                    construct(newPtr, i, std::move(ptr[i]));
                }
                for (size_type i = index; i < current; ++i) {
                    construct(newPtr, i + length, std::move(ptr[i]));
                }
                size_type count = 0;
                for (auto it = begin; it != end; ++it) {
                    construct(newPtr, index + count, *it);
                    ++count;
                }
            } else {
                insert_inplace(index, length, begin, end);
            }
            destroy_all();
            deallocate();
            ptr = newPtr;
            current += length;
            max = newCapacity;
        }

        void make_space(const size_type index, const size_type length) {
            for (size_type i = current; i > index; --i) {
                construct(ptr, i + length - 1, std::move(ptr[i - 1]));
                destroy(i - 1);
            }
        }

        void assign_from(const vector& other) {
            destroy_all();
            clear();

            if (other.size() > max) {
                realloc(other.size());
            }
            for (int i = 0; i < other.size(); ++i) {
                construct(ptr, i, data()[i]);
            }
            current = other.size();
        }

        size_type index_of(T* index) {
            cexpr::require(index <= ptr + current && index >= ptr);
            return index - ptr;
        }

        size_type index_of(size_type index) {
            cexpr::require(index >= 0 && index <= current);
            return index;
        }

        size_type index_of(const vector_iterator<T>& iterator) {
            cexpr::require(iterator.data() >= ptr && iterator.data() <= ptr + current);
            return iterator - begin();
        }

        size_type index_of(const vector_iterator<const T>& iterator) {
            cexpr::require(iterator.data() >= ptr && iterator.data() <= ptr + current);
            return iterator - begin();
        }

        void move_by_always_equal(vector& other) {
            ptr = other.ptr;
            current = other.current;
            max = other.max;
            reallocSchema = std::move(other.reallocSchema);

            other.ptr = nullptr;
            other.current = 0;
            other.max = 0;
        }

        void move_by_propagate(vector& other) {
            allocator = std::move(other.allocator);
            ptr = other.ptr;
            current = other.current;
            max = other.max;
            reallocSchema = std::move(other.reallocSchema);

            other.ptr = nullptr;
            other.current = 0;
            other.max = 0;
        }

        decltype(auto) copy_allocator(const vector& other) {
            if constexpr (alloc_traits::is_always_equal::value || alloc_traits::propagate_on_container_copy_assignment::value) {
                return other.allocator;
            } else {
                return Alloc();
            }
        }
        T* ptr = 0;
        size_type current = 0;
        size_type max = 0;
        Alloc allocator;
        ReallocationSchema reallocSchema;
    public:
        using iterator = vector_iterator<T>;

        vector() = default;

        vector(const size_type reserve) : max(static_cast<size_type>(reserve)) {
            ptr = allocator.allocate(reserve);
        }

        vector(std::initializer_list<T> items) {
            insert(0, items);
        }

        template <typename TAlloc>
        requires std::constructible_from<Alloc, TAlloc>
        vector(TAlloc&& allocator, const size_t capacity = 0) : allocator(std::forward<TAlloc>(allocator)) {
            if (capacity) {
                reserve(capacity);
            }
        }

        vector(const vector& other) = delete;

        vector& operator=(const vector& other) = delete;

        vector(vector&& other) noexcept : current(other.current), max(other.max), reallocSchema(other.reallocSchema), ptr(other.ptr) {
            if constexpr (alloc_traits::is_always_equal::value) {
                allocator = other.allocator;
            } else if constexpr (alloc_traits::propagate_on_container_move_assignment::value) {
                allocator = std::move(other.allocator);
            } else {
                static_assert(false, "Allocator Does not support move semantics");
            }
            other.ptr = nullptr;
            other.current = 0;
            other.max = 0;
        }

        vector& operator=(vector&& other) noexcept {
            if (this != &other) {
                release();

                current = other.current;
                max = other.max;
                reallocSchema = std::move(other.reallocSchema);
                ptr = other.ptr;
                other.ptr = nullptr;
                other.current = 0;
                other.max = 0;

                if constexpr (alloc_traits::is_always_equal::value) {
                    allocator = other.allocator;
                } else if constexpr (alloc_traits::propagate_on_container_move_assignment::value) {
                    allocator = std::move(other.allocator);
                } else {
                    static_assert(false, "Allocator Does not support move semantics");
                }
            }
            return *this;
        }

        ~vector() {
            clear();
            deallocate();
        }

        void reallocate() {
            realloc(max);
        }
        
        template <typename TT>
        requires std::constructible_from<T, TT>
        T& push_back(TT&& val) {
            ensure_has_memory(1);
            return *construct(current++, std::forward<TT>(val));
        }

        template <typename Index, typename... Args>
        requires std::constructible_from<T, Args...>
        T& emplace(Index&& idx, Args&&... args) {
            return insert(idx, T(std::forward<Args>(args)...));
        }

        template <typename Index, typename TT>
        requires std::constructible_from<T, TT>
        T& insert(Index&& idx, TT&& value) {
            size_type index = index_of(idx);

            if (!ensure_has_memory_reserve(1, index, 1)) {
                for (size_type i = current; i > index; --i) {
                    construct(ptr, i, std::move(ptr[i - 1]));
                    destroy(i - 1);
                }
            }
            ++current;
            construct(ptr, index, std::forward<TT>(value));
            return ptr[index];
        }

        template <typename Index, typename R1, typename R2>
        auto insert(Index&& idx, R1&& begin, R2&& end) {
            size_type index = index_of(idx);

            if (index > current) {
                index = current;
            }
            const size_t distance = std::distance(begin, end);

            if (!has_memory(distance)) {
                realloc_insert(index, distance, begin, end);
            } else {
                insert_inplace(index, distance, begin, end);
            }
            return make_iterator(index);
        }

        template <typename Index, typename T2>
        requires std::ranges::range<T2>
        auto insert(Index&& index, T2&& range) {
            return insert(std::forward<Index>(index), range.begin(), range.end());
        }

        template <typename... Args>
        T& emplace_back(Args&&... args) {
            ensure_has_memory(1);
            construct(ptr, current++, std::forward<Args>(args)...);
            return ptr[current - 1];
        }

        template <typename Index>
        auto erase(const Index idx) {
            size_type index = index_of(idx);
            cexpr::require(index < current && index >= 0);

            for (size_type i = index; i < current; ++i) {
                destroy(i);
                construct(ptr, i, std::move(ptr[i + 1]));
            }
            --current;
            return make_iterator(index);
        }

        template <typename I1, typename I2>
        auto erase(I1&& first, I2&& last) {
            size_type i1 = index_of(first);
            size_type i2 = index_of(last);

            for (size_type i = i1; i < i2; ++i) {
                destroy(i);
            }
            
            size_type remaining = current - i2;
            for (size_type i = 0; i < remaining; ++i) {
                construct(ptr, i + i1, std::move(ptr[i + i2]));
                destroy(i + i2);
            }
            size_type distance = i2 - i1;
            current -= distance;

            return make_tterator(i1);
        }

        void pop_back() {
            cexpr::require(current > 0);
            destroy(--current);
        }

        T pop_retrieve() {
            cexpr::require(current > 0);
            T temp = ptr[--current];
            destroy(current);
            return temp;
        }

        void reserve(const size_type capacity) {
            if (max < capacity) {
                realloc(capacity);
            }
        }

        void expand_by(const size_type elements) {
            realloc(max + elements);
        }

        void shrink_to_fit() {
            if (current < max) {
                if constexpr (IsShrinkableAllocator<Alloc>) {
                    auto* newPtr = allocator.shrink_allocate(ptr, current, max);

                    if (ptr != newPtr) {
                        realloc(current);
                    }
                } else {
                    realloc(current);
                }
                max = current;
            }
        }

        decltype(auto) operator [] (this auto&& self, const size_type index) {
            cexpr::require(index < self.current, [&] {
                using VecT = std::remove_reference_t<decltype(self)>;
                std::cout << "[" << cexpr::name_of<VecT>
                          << "] Index out of bounds: index " << index
                          << " >= size " << self.current
                          << " (capacity " << self.max
                          << ")\n";
            });
            return std::forward_like<decltype(self)>(self.ptr[index]);
        }


        decltype(auto) at(this auto&& self, const size_type index) {
            cexpr::require(index < self.current, [&] {
                using VecT = std::remove_reference_t<decltype(self)>;
                return std::string("[")
                    + std::string(cexpr::name_of<VecT>)
                    + std::string("] Index out of bounds: index ")
                    + std::to_string(index)
                    + " >= size "
                    + std::to_string(self.current)
                    + " (capacity "
                    + std::to_string(self.max)
                    + ")\n";
            });
            return std::forward_like<decltype(self)>(self.ptr[index]);
        }

        template <typename TT = T>
        void assign(size_t count, TT&& t) {
            reserve(count);
            for (auto i = 0; i < count; ++i) {
                construct(ptr, i, t);
            }
            current = count;
        }

        void fill_remaining() {
            for (size_type i = current; i < max; ++i) {
                construct(ptr, i);
            }
            current = max;
        }

        operator range<T>() {
            return {
                ptr,
                ptr + current
            };
        }

        operator range<const T>() const {
            return {
                ptr,
                ptr + current
            };
        }

        T* data() {
            return ptr;
        }

        const T* data() const {
            return ptr;
        }

        vector_iterator<T> begin() {
            return {
                ptr
            };
        }

        vector_iterator<T> end() {
            return {
                ptr + current
            };
        }

        vector_iterator<const T> begin() const {
            return {
                ptr
            };
        }

        vector_iterator<const T> end() const {
            return {
                ptr + current
            };
        }

        size_type size() const {
            return current;
        }

        size_type capacity() const {
            return max;
        }

        bool full() const {
            return current == capacity();
        }

        const Alloc& get_allocator() const {
            return allocator;
        }

        auto& back() {            cexpr::require(current > 0);
            return ptr[current - 1];
        }

        auto& back() const {            cexpr::require(current > 0);
            return ptr[current - 1];
        }

        void clear() {
            destroy_all();
            current = 0;
        }

        void zero_out() {
            clear();
            deallocate();
            max = 0;
            ptr = nullptr;
        }

        void release() {
            clear();
            deallocate();
            max = 0;
            ptr = nullptr;
        }

        T* oprhan() {
            auto out = ptr;
            ptr = nullptr;
            current = 0;
            max = 0;
            return out;
        }

        template <typename UVal>
        T* find(const UVal& value) {
            for (auto& val : *this) {
                if (val == value) {
                    return &val;
                }
            }
            return nullptr;
        }

        template <typename UVal, typename Proj>
        T* find(const UVal& value, Proj&& proj) {
            for (auto& val : *this) {
                if (std::invoke(proj, val) == value) {
                    return &val;
                }
            }
            return nullptr;
        }

        static vector from(T* begin, size_t size, size_t capacity, Alloc alloc) {
            mem::vector vec;
            vec.current = size;
            vec.max = capacity;
            vec.ptr = begin;
            vec.allocator = alloc;

            return vec;
        }

        bool empty() const {
            return !current;
        }

        template <
            typename T,
            typename Alloc,
            typename ReallocationSchema,
            typename size_type
        > requires IsAllocator<T, Alloc>
        friend auto make_vector_from(T* begin, size_t size, size_t capacity, Alloc&& allocator);

        template <typename Vector, typename AllocArg>
        requires cexpr::is_template_of<vector, Vector>
        friend auto make_vector_like(typename Vector::value_type* begin, size_t size, size_t capacity, AllocArg&& allocator);
    };

    template <typename Vec>
    auto clone(Vec&& vector) {
        std::decay_t<Vec> vec;
        vec.reserve(vector.size());
        vec.insert(vec.end(), vector.begin(), vector.end());
        return vec;
    }

    template <
        typename T,
        typename Alloc = default_allocator<T>,
        typename ReallocationSchema = doubling_schema,
        typename size_type = size_t
    > requires IsAllocator<T, Alloc> /* Requires begin to belong to the allocator */
    auto make_vector_from(T* begin, size_t size, size_t capacity, Alloc&& allocator) {
        vector<T, Alloc, ReallocationSchema, size_type> vec;
        vec.current = size;
        vec.max = capacity;
        vec.ptr = begin;
        vec.allocator = std::forward<Alloc>(allocator);
        return vec;
    }

    template <typename Vector, typename AllocArg>
    requires cexpr::is_template_of<vector, Vector>
    auto make_vector_like(typename Vector::value_type* begin, size_t size, size_t capacity, AllocArg&& allocator) {
        std::decay_t<Vector> vec;
        vec.current = size;
        vec.max = capacity;
        vec.ptr = begin;
        vec.allocator = typename Vector::allocator_type(std::forward<AllocArg>(allocator));
        return vec;
    }

    template <typename T, template <typename> typename Alloc, typename Realloc = mem::same_alloc_schema, typename Size = size_t>
    using tvector = vector<T, Alloc<T>, Realloc, Size>;
}
