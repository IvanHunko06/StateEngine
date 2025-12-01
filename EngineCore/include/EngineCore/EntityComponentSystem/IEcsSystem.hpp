#pragma once
#include "IEngineScene.hpp"

namespace StateEngine::EngineCore::EntityComponentSystem {
	class IEcsSystem {
	public:
		virtual void update(float deltaTime, IEngineScene* scene) = 0;
	};
}