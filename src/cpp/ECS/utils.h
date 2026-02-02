#pragma once
#include <coroutine>
#include <generator>
#include <ECS/Component/ComponentRegistry.h>
#include <algorithm>

template <typename Iterator>
concept IsTypeDataIterator = requires(Iterator it) {
    it.begin();
    it.end();

    std::convertible_to<std::pair<TypeUUID, void*>, decltype(*it.begin())>;
};

template <typename Iterator>
concept IsTypeIterator = requires(Iterator it) {
    std::is_same_v<decltype(*it.begin()), TypeUUID>;
};

namespace hash {
    constexpr static size_t DEFAULT_HASH = 14695981039346656037ull;

    inline void hashNextType(TypeUUID type, size_t& hash) {
        hash ^= std::hash<TypeUUID>{}(type);
        hash *= 1099511628211ull;
    }
}

template <typename... Ranges>
size_t hash_unsigned_span(Ranges&&... spans) {
    size_t hash = hash::DEFAULT_HASH;

    ([&]{
        for (TypeUUID v : spans) {
            hash::hashNextType(v, hash);
        }
    }(), ...);
    return hash;
}

// template <typename T, typename... Ranges>
// size_t hashAliasingTypeRanges(Ranges&&... ranges) {
//     std::tuple iterPairs = std::make_tuple(
//         std::make_pair(ranges.begin(), ranges.end())...
//     );
//
//     T last = std::numeric_limits<T>::max();
//     size_t hash = hash::DEFAULT_HASH;
//
//     auto advanceIter = [&]{
//         const T* minVal = nullptr;
//         size_t minIdx = 0;
//         size_t idx = 0;
//
//         [&]<size_t... Is>(std::index_sequence<Is...>) {
//             ([&]{
//                 auto& [it, end] = std::get<Is>(iterPairs);
//                 if (it != end) {
//                     if (!minVal || *it < *minVal) {
//                         minVal = &(*it);
//                         minIdx = idx;
//                     }
//                 }
//                 ++idx;
//             }(), ...);
//         }(std::make_index_sequence<sizeof...(Ranges)>{});
//
//         if (!minVal) {
//             return false;
//         }
//
//         if (*minVal != last) {
//             last = *minVal;
//             auto& v = last;
//             hash::hashNextType(static_cast<TypeUUID>(v), hash);
//         }
//
//         size_t i = 0;
//
//         [&]<size_t... Is>(std::index_sequence<Is...>) {
//             ([&]{
//                 if (i++ == minIdx) {
//                     auto& pair = std::get<Is>(iterPairs);
//
//                     ++pair.first;
//                 }
//             }(), ...);
//         }(std::make_index_sequence<sizeof...(Ranges)>{});
//
//         return true;
//     };
//
//     while (advanceIter());
//     return hash;
// }

template <typename Range>
size_t hash_type_data_span(Range&& span) {
    size_t hash = hash::DEFAULT_HASH;

    for (auto [v, _] : span) {
        hash::hashNextType(v);
    }
    return hash;
}

template <typename T>
constexpr static auto INVALID_INDEX = std::numeric_limits<T>::max();

template <typename Iter1, typename Iter2>
class SetDifferenceIterator {
public:
    using ValueType = TypeUUID;

    using value_type = ValueType;
    using reference = const ValueType&;
    using pointer = const ValueType*;
    using iterator_category = std::input_iterator_tag;

    SetDifferenceIterator() = default;

    SetDifferenceIterator(Iter1 aBegin, Iter1 aEnd, Iter2 bBegin, Iter2 bEnd)
        : aCur(aBegin), aEnd(aEnd), bCur(bBegin), bEnd(bEnd) {
        advance();
    }

    reference operator*() const { return currentValue; }
    pointer operator->() const { return &currentValue; }

    SetDifferenceIterator& operator++() {
        advance();
        return *this;
    }

    SetDifferenceIterator operator++(int) {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    bool operator==(const SetDifferenceIterator& other) const {
        return finished;
    }

    bool operator!=(const SetDifferenceIterator& other) const {
        return !finished;
    }

private:
    Iter1 aCur, aEnd;
    Iter2 bCur, bEnd;
    ValueType currentValue;
    bool finished = false;

    void advance() {
        while (aCur != aEnd && bCur != bEnd) {
            if (*aCur < *bCur) {
                currentValue = *aCur++;
                return;
            } else if (*bCur < *aCur) {
                ++bCur;
            } else {
                ++aCur;
                ++bCur;
            }
        }

        if (aCur != aEnd) {
            currentValue = *aCur++;
        } else {
            finished = true;
        }
    }
};

template <typename Range1, typename Range2>
class SetDifferenceRange {
public:
    using Iter1 = decltype(std::begin(std::declval<Range1&>()));
    using Iter2 = decltype(std::begin(std::declval<Range2&>()));
    using Iterator = SetDifferenceIterator<Iter1, Iter2>;

    SetDifferenceRange(Range1&& a, Range2&& b)
        : aRange(std::forward<Range1>(a)), bRange(std::forward<Range2>(b)) {}

    Iterator begin() {
        return Iterator(std::begin(aRange), std::end(aRange), std::begin(bRange), std::end(bRange));
    }

    Iterator end() {
        return Iterator();
    }

private:
    Range1 aRange;
    Range2 bRange;
};

template <typename A, typename B>
auto set_difference_iterator(A&& a, B&& b) {
    return SetDifferenceRange<A, B>(std::forward<A>(a), std::forward<B>(b));
}


template <typename Iter1, typename Iter2>
class SetUnionIterator {
public:
    using ValueType = TypeUUID;

    using value_type = ValueType;
    using reference = const ValueType&;
    using pointer = const ValueType*;
    using iterator_category = std::input_iterator_tag;

    SetUnionIterator() = default;

    SetUnionIterator(Iter1 aBegin, Iter1 aEnd, Iter2 bBegin, Iter2 bEnd)
        : aCur(aBegin), aEnd(aEnd), bCur(bBegin), bEnd(bEnd)
    {
        advance();
    }

    reference operator*() const { return currentValue; }
    pointer operator->() const { return &currentValue; }

    SetUnionIterator& operator++() {
        advance();
        return *this;
    }

    SetUnionIterator operator++(int) {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    bool operator==(const SetUnionIterator& other) const {
        return finished;
    }

    bool operator!=(const SetUnionIterator& other) const {
        return !finished;
    }

private:
    Iter1 aCur, aEnd;
    Iter2 bCur, bEnd;
    ValueType currentValue;
    bool finished = false;

    void advance() {
        if (aCur == aEnd && bCur == bEnd) {
            finished = true;
            return;
        }

        if (aCur == aEnd) {
            currentValue = *bCur++;
            return;
        }

        if (bCur == bEnd) {
            currentValue = *aCur++;
            return;
        }

        if (*aCur < *bCur) {
            currentValue = *aCur++;
        } else if (*bCur < *aCur) {
            currentValue = *bCur++;
        } else {
            currentValue = *aCur;
            ++aCur;
            ++bCur;
        }
    }
};

template <typename Range1, typename Range2>
class SetUnionRange {
public:
    using Iter1 = decltype(std::begin(std::declval<Range1&>()));
    using Iter2 = decltype(std::begin(std::declval<Range2&>()));

    using Iterator = SetUnionIterator<Iter1, Iter2>;

    SetUnionRange(Range1&& a, Range2&& b)
        : aRange(std::forward<Range1>(a)), bRange(std::forward<Range2>(b)) {}

    Iterator begin() {
        return Iterator(std::begin(aRange), std::end(aRange), std::begin(bRange), std::end(bRange));
    }

    Iterator end() {
        return Iterator();
    }

private:
    Range1 aRange;
    Range2 bRange;
};

template <typename A, typename B>
auto set_union_iterator(A&& a, B&& b) {
    return SetUnionRange<A, B>(std::forward<A>(a), std::forward<B>(b));
}

template <typename... Nums>
size_t hash_combine(Nums&&... nums) {
    auto hash_combine_impl = [](size_t& seed, size_t value) {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    };
    size_t seed = 0;
    (hash_combine_impl(seed, static_cast<size_t>(nums)), ...);
    return seed;
}