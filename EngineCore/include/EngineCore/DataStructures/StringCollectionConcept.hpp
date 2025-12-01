#pragma once
#include <concepts>

template <typename T>
concept StringCollection = requires(const char* str) {
	{ T::getOrCreateSharedString(str) } -> std::convertible_to<const char*>;
	{ T::incrementRefCount(str) } -> std::convertible_to<void>;
	{ T::decrementRefCount(str) } -> std::convertible_to<void>;
};