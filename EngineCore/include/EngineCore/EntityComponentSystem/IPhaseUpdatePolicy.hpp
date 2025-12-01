#pragma once
#include "EngineCore/EngineUpdatePhase.hpp"

namespace StateEngine::EngineCore::EntityComponentSystem {
	class IPhaseUpdatePolicy {
	public:
		using PFN_UpdateSystemsCallback = void(*)(void* context, float deltaTime);
		virtual void executeUpdatePolicy(void* context, float realDeltaTime, EngineUpdatePhase phase, PFN_UpdateSystemsCallback updateCallback) = 0;
	};
}