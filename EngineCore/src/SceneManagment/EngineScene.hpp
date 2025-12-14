#pragma once
#include "EntityComponentSystem/ZArchetypeComponentRegistry.hpp"
#include "EngineCore/SceneManagment/IEngineScene.hpp"
#include "EngineCore/EntityComponentSystem/IEcsSystem.hpp"

using StateEngine::EngineCore::SceneManagment::IEngineScene;
using StateEngine::EngineCore::EntityComponentSystem::ZArchetypeComponentRegistry;

namespace StateEngine::EngineCore::SceneManagment {
	class EngineScene : public IEngineScene{
	private:
		ZArchetypeComponentRegistry registry_;
		//ZBuffer<IEcsSystem> systems_;
	};
}