#pragma once
#include "EngineCore/DataStructures/ZFunction.hpp"
#include <atomic>

using StateEngine::EngineCore::DataStructures::ZFunction;
namespace StateEngine::EngineCore::JobSystem {
	using JobFunction = ZFunction<void(size_t rangeBegin, size_t rangeEnd, const std::atomic_bool* isCanceled)>;
}