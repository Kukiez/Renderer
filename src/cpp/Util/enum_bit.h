#pragma once
#include <type_traits>

template <typename Enum>
requires std::is_enum_v<Enum>
constexpr Enum operator&(Enum lhs, Enum rhs) {
    using Underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(
        static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs)
    );
}

template <typename Enum>
requires std::is_enum_v<Enum>
constexpr Enum operator~(Enum e) {
    using Underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(~static_cast<Underlying>(e));
}

template <typename Enum>
requires std::is_enum_v<Enum>
constexpr Enum& operator|=(Enum& lhs, Enum rhs) {
    using Underlying = std::underlying_type_t<Enum>;
    lhs = static_cast<Enum>(static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs));
    return lhs;
}

template <typename Enum>
requires std::is_enum_v<Enum>
constexpr Enum operator|(Enum lhs, Enum rhs) {
    using Underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs));
}


template <typename Enum>
requires std::is_enum_v<Enum>
constexpr Enum& operator&=(Enum& lhs, Enum rhs) {
    using Underlying = std::underlying_type_t<Enum>;
    lhs = static_cast<Enum>(static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs));
    return lhs;
}

template <typename Enum>
requires std::is_enum_v<Enum>
constexpr Enum operator - (Enum lhs, Enum rhs) {
    using Underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<Underlying>(lhs) - static_cast<Underlying>(rhs));
}

template <typename Enum>
requires std::is_enum_v<Enum>
constexpr Enum operator + (Enum lhs, Enum rhs) {
    using Underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<Underlying>(lhs) + static_cast<Underlying>(rhs));
}

template <typename Enum>
bool check_enum_bit(Enum check, Enum bit) {
    return (check & bit) == bit;
}

template <typename Enum>
class EnumFlags {
    Enum e;
public:
    EnumFlags() = default;
    EnumFlags(Enum e) : e(e) {}

    operator Enum() const { return e; }

    bool has(Enum f) const { return (e & f) == f; }
};
