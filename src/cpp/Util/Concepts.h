#pragma once

#include <iostream>
#include <type_traits>

template <typename T>
concept Numeric = std::is_arithmetic_v<std::decay_t<T>> || std::is_convertible_v<std::decay_t<T>, int>;

template <typename T>
concept Enum = std::is_enum_v<std::decay_t<T>>;