#pragma once
namespace StateEngine::EngineCore::SceneManagment {
	class IEngineScene;
}
using StateEngine::EngineCore::SceneManagment::IEngineScene;
namespace StateEngine::EngineCore::EntityComponentSystem {
	class IEcsSystem {
	public:
		virtual void Update(float deltaTime, IEngineScene* scene) = 0;
	};
}