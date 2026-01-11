#pragma once
#include "EngineCore/DataStructures/ZFunction.hpp"

using StateEngine::EngineCore::DataStructures::ZFunction;
namespace StateEngine::EngineCore::EntityComponentSystem {
    struct EcsEntity;
	using ForeachCallbackFunction = ZFunction<void(const EcsEntity* entities, void** components, size_t entitiesCount)>;
}