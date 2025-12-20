#pragma once
#include "EngineCore/DataStructures/ZString.hpp"
#include <concepts>

template <typename T>
concept HasToStringFunction = requires(const char* format) {
    { T::ToString(format) } -> std::convertible_to<StateEngine::EngineCore::DataStructures::ZString>;
};