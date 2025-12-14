#pragma once
#include "EngineCore/EngineUpdatePhase.hpp"
#include "EngineCore/DataStructures/ZFunction.hpp"

using StateEngine::EngineCore::DataStructures::ZFunction;
namespace StateEngine::EngineCore::SceneManagment {
	using UpdateSystemCallback = ZFunction<void(float deltaTime)>;
	using PhaseUpdatePolicy = ZFunction<void(float realDeltaTime, EngineUpdatePhase phase, const UpdateSystemCallback& updateCallback)>;
}