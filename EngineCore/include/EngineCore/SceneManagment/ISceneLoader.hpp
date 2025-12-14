#pragma once
#include "IEngineScene.hpp"

namespace StateEngine::EngineCore::SceneManagment {
	class ISceneLoader {
	public:
		/// <summary>
		/// A scene loading method that will asynchronously load a scene. 
		/// This function will run with low priority inside Job System, 
		/// not from within the main thread. In the method, you can instantiate 
		/// entities or load resources.
		/// </summary>
		/// <param name="scene">
		/// The context of the scene that needs to be filled. 
		/// It is safe to populate it on this thread. 
		/// The changes will be applied after this function completes.
		/// </param>
		virtual void OnSceneLoading(IEngineScene* scene) = 0;

		/// <summary>
		/// Scene unloading method. This is where all resources can be freed. 
		/// This method runs in the Job System with low priority, not the main thread.
		/// Here you can unload resources
		/// </summary>
		/// <param name="scene"></param>
		virtual void OnSceneUnloading(IEngineScene* scene) = 0;
	};
}