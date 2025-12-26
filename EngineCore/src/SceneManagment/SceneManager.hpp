#pragma once
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/DataStructures/ZString.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/SmartPointers/ZUniquePointer.hpp"
#include "EngineCore/JobSystem/JobHandle.hpp"
#include "EngineScene.hpp"
#include "EngineCore/SceneManagment/ISceneLoader.hpp"

using StateEngine::EngineCore::DataStructures::ZHashMap;
using StateEngine::EngineCore::DataStructures::ZString;
using StateEngine::EngineCore::SmartPointers::ZUniquePointer;
using StateEngine::EngineCore::JobSystem::JobHandle;
namespace StateEngine::EngineCore::SceneManagment {
	class SceneManager {
	private:
		struct ActiveSceneEntry {
			EngineScene* scene;
			int updateOrder;
			int renderOrder;
			bool isPaused;
		};
		struct LoadingSceneEntry {
			JobHandle jobHandle;
			ZUniquePointer<EngineScene> scene;
		};

	private:
		static ZHashMap<ZString, ZUniquePointer<EngineScene>> loadedScenes_;
		static DataStructures::ZBuffer<ActiveSceneEntry> activeScenes_;
		static DataStructures::ZBuffer<LoadingSceneEntry> asyncLoadingScenes_;
		static ZHashMap<ZString, ISceneLoader*> sceneLoaders;

	public:
		/// <summary>
		/// Starts asynchronous scene loading. Will be called during the LoadScene event. 
		/// Called from the main thread during FlushEvents; no synchronization is required. 
		/// After loading, publishes the SceneLoaded event.
		/// </summary>
		/// <param name="name">The name of the scene to be loaded</param>
		void LoadSceneAsync(const ZString& name);

		/// <summary>
		/// Begins synchronous scene loading. Called during the SceneLoad event. 
		/// Called from the main thread during FlushEvents and will block until completed. 
		/// No synchronization is required.
		/// </summary>
		/// <param name="name">The name of the scene to be loaded</param>
		void LoadSceneImmediate(const ZString& name);

		/// <summary>
		/// Initiates an asynchronous scene unload. Called during the UnloadScene event. 
		/// Called from the main thread during FlushEvents; no synchronization is required. 
		/// </summary>
		/// <param name="name">The name of the scene to be unloaded</param>
		void UnloadScene(const ZString& name);

		/// <summary>
		/// Adds a scene to active ones. Called during the ActivateScene event.
		/// Called from the main thread during FlushEvents; no synchronization is required.
		/// Publishes the SceneLoaded event.
		/// </summary>
		/// <param name="name"></param>
		void ActivateScene(const ZString& name, int updateOrder, int renderOrder);

		/// <summary>
		/// Adds a scene to active ones. Called during the DeactivateScene event. 
		/// Called from the main thread during FlushEvents; no synchronization is required.
		/// Publishes the SceneUnloaded event.
		/// </summary>
		/// <param name="name"></param>
		void DeactivateScene(const ZString& name);

		/// <summary>
		/// Checks all asynchronous scene loadings for completion via the job handle.
		/// All completed tasks are moved from asyncLoadingScenes_ to loadedScenes_.
		/// Called exclusively from the main thread
		/// </summary>
		void CheckAsyncLoadedScenes();

		/// <summary>
		/// Registers event listeners during engine startup.
		/// </summary>
		void RegisterEventListeners();

		/// <summary>
		/// Updates all active scenes. Called from the game loop.
		/// </summary>
		/// <param name="realDeltaTime"></param>
		void UpdateActiveScenes(float realDeltaTime);

		/// <summary>
		/// Pauses the scene
		/// </summary>
		/// <param name="name"></param>
		/// <param name="paused"></param>
		void SetScenePaused(const ZString& name, bool paused);
	};
}