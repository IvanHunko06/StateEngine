#pragma once
//#include "EntityComponentSystem/ZComponentsRegistry.hpp"
#include "EngineCore/EntityComponentSystem/IEngineScene.hpp"
#include "EngineCore/EntityComponentSystem/IEcsSystem.hpp"

//using StateEngine::EngineCore::EntityComponentSystem::ZComponentsRegistry;
using StateEngine::EngineCore::EntityComponentSystem::IEcsSystem;
using StateEngine::EngineCore::EntityComponentSystem::IEngineScene;

namespace StateEngine::EngineCore {
	class ZEngineScene : public IEngineScene{
	private:
		//ZComponentsRegistry registry_;
		//ZBuffer<IEcsSystem> systems_;
	};
}