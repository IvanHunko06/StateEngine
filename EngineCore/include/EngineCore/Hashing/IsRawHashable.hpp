#pragma once
#include <concepts>
#include <type_traits>

template <typename T>
concept IsRawHashable = std::is_trivial_v<T>;