#pragma once
#include "EngineCore/DataStructures/ZFunction.hpp"
using StateEngine::EngineCore::DataStructures::ZFunction;

namespace StateEngine::EngineCore::EventBus {
	using EventCallback = ZFunction<bool(void*)>;
}