#pragma once
#include "alloc.h"

namespace mem 
{
    class bitset_iterator_sentinel_end {};

    template <typename T>
    class bitset_iterator {
        const T* data = 0;
        size_t wordsCount;
        size_t wordIndex = 0;
        size_t currentWord = 0;
        size_t currentBit = -1;

        bool load_next_word() {
            while (wordIndex < wordsCount) {
                if (data[wordIndex] != 0) {
                    currentWord = data[wordIndex];
                    currentBit = -1;
                    return true;
                }
                ++wordIndex;
            }
            return false;
        }

        void advance() {
            if (currentWord == 0) {
                ++wordIndex;
                if (!load_next_word()) return;
            }
            currentBit = std::countr_zero(currentWord);
            currentWord &= currentWord - 1;
        }
    public:
        bitset_iterator(const T* data, size_t words) : data(data), wordsCount(words) {
            if (data) {
                currentWord = data[0];
                advance();
            }
        }

        bitset_iterator& operator ++ () {
            advance();
            return *this;
        }

        bitset_iterator operator ++ (int) {
            auto copy = *this;
            advance();
            return copy;
        }

        size_t operator * () const {
            return wordIndex * sizeof(T) * 8 + currentBit;
        }

        bool operator == (const bitset_iterator_sentinel_end& other) const {
            return wordIndex == wordsCount;
        }

        bool operator != (const bitset_iterator_sentinel_end& other) const {
            return wordIndex != wordsCount;
        }
    };

    class bit_reference {
        size_t* word;
        uint8_t bit;
    public:
        bit_reference(std::nullptr_t) : word(nullptr), bit(0) {}
        bit_reference(size_t* word, uint8_t bit) : word(word), bit(bit) {}

        operator bool () const {
            return (*word >> bit) & 1;
        }

        bit_reference& operator = (const bool value) {
            if (value) {
                *word |= 1ull << bit;
            } else {
                *word &= ~(1ull << bit);
            }
            return *this;
        }
    };

    template <int N>
    struct bitset_fixed {
        size_t data[1 + N / 64];

        size_t* allocate(const size_t count) {
            return data;
        }

        void deallocate(size_t* ptr, size_t) {}
    };

    template <
        typename Alloc = default_allocator<size_t>, 
        typename BitType = size_t, 
        typename WordsType = size_t
    >
    class bitset {
        constexpr static auto bits = sizeof(BitType) * 8;
        constexpr static auto FULL = std::numeric_limits<WordsType>::max();

        BitType* words = 0;
        WordsType wordsCount = 0;
        Alloc allocator;
    public:
        using AllocTraits = mem::allocator_traits<Alloc>;

        bitset() = default;

        size_t size() const {
            return wordsCount * bits;
        }

        template <typename TAlloc>
        requires std::constructible_from<Alloc, TAlloc> 
        bitset(TAlloc&& allocator, size_t capacity = 0) :
            allocator(std::forward<TAlloc>(allocator))
        {
            if (capacity != 0) {
                reserve(capacity);
            }
        }

        bitset(const size_t forBits) {
            reserve(forBits);
        }

        bit_reference operator [] (const size_t idx) {
            cexpr::require(idx < wordsCount * bits);
            return bit_reference{&words[idx / bits], idx % bits};
        }

        bitset(const bitset& other) = delete;
        bitset& operator = (const bitset& other) = delete;

        bitset(bitset&& other) noexcept {
            if constexpr (AllocTraits::is_always_equal::value) {
                allocator = other.allocator;
            } else {
                allocator = std::move(other.allocator);
            }
            words = other.words;
            wordsCount = other.wordsCount;
            other.words = 0;
            other.wordsCount = 0;
        }

        bitset& operator = (bitset&& other) noexcept {
            if (this != &other) {
                if constexpr (AllocTraits::is_always_equal::value) {
                    allocator = other.allocator;
                } else {
                    allocator = std::move(other.allocator);
                }
                words = other.words;
                wordsCount = other.wordsCount;
                other.words = 0;
                other.wordsCount = 0;
            }
            return *this;
        }

        ~bitset() {
            allocator.deallocate(words, wordsCount);
        }

        void reserve(const size_t forBits) {
            if (forBits > wordsCount * bits) {
                size_t newWordsCount = ((forBits + bits - 1) & ~(bits - 1)) / bits;
                auto* newWords = allocator.allocate(newWordsCount);

                if (words) {
                    allocator.deallocate(words, wordsCount);
                    memcpy(newWords, words, sizeof(BitType) * wordsCount);                   
                }
                memset(newWords + sizeof(BitType) * wordsCount, 0, (newWordsCount - wordsCount) * sizeof(BitType)); 
                wordsCount = newWordsCount;
                words = newWords;
            }
        }

        void set(const size_t idx) noexcept {
            cexpr::require(idx < wordsCount * bits);
            words[idx / bits] |= BitType(1) << (idx % bits);
        }

        void set_and_expand(const size_t idx) {
            if (idx >= wordsCount * bits) {
                reserve(idx + 1);
            }
            words[idx / bits] |= BitType(1) << (idx % bits);
        }

        void set_range(size_t first, size_t last) {
            if (first >= last) return;

            last = std::min(last, size());
            first = std::min(first, last);
            if (first == last) return;

            const size_t firstWord = first / bits;
            const size_t lastWord  = (last - 1) / bits;

            const uint32_t firstBit = static_cast<uint32_t>(first % bits);
            const uint32_t lastBit  = static_cast<uint32_t>((last - 1) % bits);

            if (firstWord == lastWord) {
                uint32_t span = lastBit - firstBit + 1;
                uint64_t mask = (span == 64) ? FULL : ((FULL >> (64 - span)) << firstBit);
                words[firstWord] |= mask;
                return;
            }

            if (firstBit == 0) {
                words[firstWord] = FULL;
            } else {
                words[firstWord] |= (FULL << firstBit);
            }

            for (size_t i = firstWord + 1; i < lastWord; ++i) {
                words[i] = FULL;
            }

            if (lastBit == 63) {
                words[lastWord] = FULL;
            } else {
                words[lastWord] |= (FULL >> (63 - lastBit));
            }
        }

        void reset(size_t idx) noexcept {
            cexpr::require(idx < wordsCount * bits);
            words[idx / bits] &= ~(BitType(1)) << (idx % bits);
        }

        void reset_checked(const size_t idx) {
            if (idx < wordsCount * bits) return;
            words[idx / bits] &= ~(BitType(1)) << (idx % bits);
        }

        bool test(size_t idx) noexcept {
            if (idx >= wordsCount * bits) return false;
            return (words[idx / bits] >> (idx % bits)) & 1;
        }

        void free() noexcept {
            if (words) {
                allocator.deallocate(words, wordsCount);
                words = nullptr;
                wordsCount = 0;                
            }
        }

        void clear() noexcept {
            memset(words, 0, sizeof(BitType) * wordsCount);
        }

        void set_all() {
            memset(words, 0xFF, sizeof(BitType) * wordsCount);
        }

        size_t capacity() {
            return wordsCount * bits;
        }

        template <typename TAlloc>
        requires std::constructible_from<Alloc, TAlloc> 
        void set_allocator(TAlloc&& alloc) {
            allocator = Alloc(std::forward<TAlloc>(alloc));
        }

        auto begin() {
            return bitset_iterator{words, wordsCount};
        }

        auto end() {
            return bitset_iterator_sentinel_end{};
        }

        auto data() {
            return words;
        }
    };

    template <std::integral T>
    class bitset_view {
        static constexpr T bits = sizeof(T) * 8;
        T* words;
        size_t wordsCount;
    public:
        bitset_view(T* words, size_t wordsCount) : words(words), wordsCount(wordsCount) {}

        void set(const T idx) noexcept {
            cexpr::require(idx < wordsCount * bits);
            words[idx / bits] |= T(1) << (idx % bits);
        }

        void reset(const T idx) noexcept {
            cexpr::require(idx < wordsCount * bits);
            words[idx / bits] &= ~(T(1)) << (idx % bits);
        }

        bool test(const T idx) const {
            if (idx >= wordsCount * bits) return false;
            return (words[idx / bits] >> (idx % bits)) & 1;
        }

        void clear() noexcept {
            std::memset(words, 0, sizeof(T) * wordsCount);
        }

        size_t count() const {
            size_t count = 0;
            for (auto bit : *this) {
                ++count;
            }
            return count;
        }

        auto begin() const {
            return bitset_iterator{words, wordsCount};
        }

        auto end() const {
            return bitset_iterator_sentinel_end{};
        }

        auto data() {
            return words;
        }
    };

    template <typename Alloc>
    auto make_bitset(Alloc&& allocator, size_t bits) {
        return bitset<std::decay_t<Alloc>>(std::forward<Alloc>(allocator), bits);
    }

    struct bitset_iteratable {
        bitset_iterator<size_t> myBegin;

        bitset_iteratable(const size_t* data, size_t words) : myBegin(data, words) {}

        auto begin() const {
            return myBegin;
        }

        constexpr static auto end() {
            return bitset_iterator_sentinel_end{};
        }
    };

    inline auto make_bitset_iterator(const size_t* words, const size_t wordCount) {
        return bitset_iteratable(words, wordCount);
    }
}
