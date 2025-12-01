#pragma once
#include <concepts>

template<typename TFrom, typename TTo>
concept ConvertibleToItem = std::is_constructible_v<TTo, TFrom>;