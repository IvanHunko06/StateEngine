#pragma once
#include <concepts>
#include <type_traits>


template<typename T>
concept HasHashFunction =
std::is_class_v<T> &&
!std::is_union_v<T> &&
requires(T obj) {
	{ obj.Hash() }->std::convertible_to<size_t>;
};