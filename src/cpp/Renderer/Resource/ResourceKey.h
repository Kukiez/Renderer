#pragma once
#include <limits>
#include <type_traits>
#include <concepts>

class UnsignedKeyBase {
public:
    static constexpr auto NUM_ID_BITS = 31;
    static constexpr auto NUM_GEN_BITS = 8;
protected:
    static constexpr auto INVALID = std::numeric_limits<unsigned>::max() / (1 + (32 - NUM_ID_BITS));
    unsigned key : NUM_ID_BITS;
public:
    constexpr UnsignedKeyBase() : key(INVALID) {}

    template <typename T>
    constexpr explicit UnsignedKeyBase(T key) : key(key) {}

    template <std::integral T>
    requires (!std::is_same_v<T, bool>)
    constexpr operator T() const {
        return static_cast<T>(key);
    }

    template <std::convertible_to<unsigned> T>
    constexpr bool operator > (const T& other) const {
        return key > other;
    }

    template <std::convertible_to<unsigned> T>
    constexpr bool operator < (const T& other) const {
        return key < other;
    }

    constexpr bool operator > (const UnsignedKeyBase& other) const {
        return key > other.key;
    }

    constexpr bool operator < (const UnsignedKeyBase& other) const {
        return key < other.key;
    }

    constexpr bool isValid() const {
        return key != INVALID;
    }

    constexpr bool operator == (const UnsignedKeyBase& other) const {
        return key == other.key;
    }

    constexpr bool operator != (const UnsignedKeyBase& other) const {
        return key != other.key;
    }

    constexpr unsigned index() const {
        return key;
    }

    constexpr unsigned id() const {
        return key;
    }
};