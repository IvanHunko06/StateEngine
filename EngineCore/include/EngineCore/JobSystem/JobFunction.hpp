#pragma once
#include "EngineCore/DataStructures/ZFunction.hpp"

using StateEngine::EngineCore::DataStructures::ZFunction;
namespace StateEngine::EngineCore::JobSystem {
	using JobFunction = ZFunction<void(size_t, size_t)>;
}