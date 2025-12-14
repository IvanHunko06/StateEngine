#pragma once
#include "EngineCore/DataStructures/ZFunction.hpp"

using StateEngine::EngineCore::DataStructures::ZFunction;
namespace StateEngine::EngineCore::EntityComponentSystem {
	using ForeachCallbackFunction = ZFunction<void(void** components, size_t entitiesCount)>;
}