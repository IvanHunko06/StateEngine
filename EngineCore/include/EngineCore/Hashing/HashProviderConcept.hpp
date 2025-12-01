#pragma once
#include <concepts>
#include <cstdint>

template<typename T>
concept HashProvider = requires(const void* data, size_t dataSize){
	{ T::hashBytes(data, dataSize) } -> std::convertible_to<size_t>;
};