#pragma once
#include <array>
#include <functional>

namespace mem {
    template <typename T>
    struct pointer_range {
        T* first, *last;

        auto begin() {
            return first;
        }

        auto end() {
            return last;
        }

        auto begin() const {
            return first;
        }

        auto end() const {
            return last;
        }

        auto data() {
            return first;
        }
    };

    template <typename T, auto Proj>
    struct pointer_proj_iterator {
        using projected = decltype(std::invoke(Proj, *std::declval<T*>()));
        using value_type = projected;
        using decayed_value_type = std::decay_t<projected>;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = projected;
        using iterator_category = std::random_access_iterator_tag;
    private:
        T* begin;
    public:
        pointer_proj_iterator() = default;
        pointer_proj_iterator(T* begin) : begin(begin) {}
        
        bool operator == (const pointer_proj_iterator& other) const {
            return begin == other.begin;
        }

        bool operator != (const pointer_proj_iterator& other) const {
            return begin != other.begin;
        }

        bool operator > (const pointer_proj_iterator& other) const {
            return begin > other.begin;
        }

        bool operator < (const pointer_proj_iterator& other) const {
            return begin < other.begin;
        }

        bool operator >= (const pointer_proj_iterator& other) const {
            return begin >= other.begin;
        }

        bool operator <= (const pointer_proj_iterator& other) const {
            return begin <= other.begin;
        }

        pointer_proj_iterator operator + (const difference_type count) const {
            return {
                begin + count
            };
        }

        friend pointer_proj_iterator operator + (const difference_type count, const pointer_proj_iterator& it) {
            return it + count;
        }

        pointer_proj_iterator& operator += (const difference_type count) {
            begin += count;
            return *this;
        }
        
        pointer_proj_iterator operator - (const difference_type count) const {
            return {
                begin - count
            };
        }

        difference_type operator - (const pointer_proj_iterator& other) const {
            return begin - other.begin;
        }

        pointer_proj_iterator& operator -= (const difference_type count) {
            begin -= count;
            return *this;
        }

        pointer_proj_iterator& operator ++() {
            ++begin;
            return *this;
        }

        pointer_proj_iterator operator ++(int) {
            auto temp = *this;
            ++begin;
            return temp;
        }

        pointer_proj_iterator& operator --() {
            --begin;
            return *this;
        }

        pointer_proj_iterator operator --(int) {
            auto tmp = *this;
            --begin;
            return tmp;
        }

        reference operator *() {
            return std::invoke(Proj, *begin);
        }
        
        const reference operator *() const {
            return std::invoke(Proj, *begin);
        }

        pointer operator->() { return begin; }
        const pointer operator->() const { return begin; }

        reference operator[](difference_type diff) { return std::invoke(Proj, begin[diff]); }
        const reference operator[](difference_type diff) const { return std::invoke(Proj, begin[diff]); }
    };

    template <typename T, auto Proj>
    struct pointer_proj_iteratable {
        T* first, *last;

        auto begin() {
            return pointer_proj_iterator<T, Proj>{first};
        }

        auto end() {
            return pointer_proj_iterator<T, Proj>{last};
        }

        auto begin() const {
            return pointer_proj_iterator<const T, Proj>{first};
        }

        auto end() const {
            return pointer_proj_iterator<const T, Proj>{last};
        }
    };

    template <typename T>
    struct range {
        T* first;
        T* last;

        range() : first(nullptr), last(nullptr) {}
        range(T* begin, T* end) : first(begin), last(end) {}
        range(T* begin, size_t count) : first(begin), last(begin + count) {}

        operator range<const T>() const {
            return {first, last};
        }

        size_t length() const {
            return last - first;
        }

        T& operator [] (const size_t idx) {
            return first[idx];
        }

        const T& operator [] (const size_t idx) const {
            return first[idx];
        }

        auto begin() {
            return first;
        }

        auto end() {
            return last;
        }

        auto begin() const {
            return first;
        }

        auto end() const {
            return last;
        }

        auto data() {
            return first;
        }

        auto data() const {
            return first;
        }

        size_t size() const {
            return last - first;
        }

        bool empty() const {
            return first == last;
        }
    };

    template <typename T>
    auto make_range(T* begin, T* end) {
        return range<T>{begin, end};
    }

    template <typename T>
    auto make_const_range(T* begin, T* end) {
        return range<const T>{begin, end};
    }

    template <typename T>
    auto as_range(T& ref) {
        return range<T>{&ref, &ref + 1};
    }

    template <typename T, typename Proj>
    auto make_range(T* begin, T* end, Proj&&) {
        return pointer_proj_iteratable<T, std::decay_t<Proj>{}>(begin, end);
    }

    template <typename T, size_t N>
    auto make_range(std::array<T, N>& arr) {
        return range<T>{arr.data(), arr.data() + N};
    }

    template <typename T, size_t N>
    auto make_range(std::array<const T, N>& arr) {
        return range<const T>{arr.data(), arr.data() + N};
    }

    template <typename T, size_t N>
    auto make_range(const std::array<T, N>& arr) {
        return range<const T>{arr.data(), arr.data() + N};
    }

    template <typename T, auto Proj>
    auto make_range(T* begin, const size_t count) {
        return pointer_proj_iteratable<T, Proj>(begin, begin + count);
    }

    template <typename T>
    auto make_null_range() {
        return range<T>{};
    }

    template <typename T>
    auto make_range(T* begin, size_t len) {
        return range<T>{begin, begin + len};
    }
}