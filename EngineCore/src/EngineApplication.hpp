#pragma once
#include "EngineCore/ApplicationConfigurations/EngineBuilder.hpp"
#include "EngineCore/IModule.hpp"
#include "EngineCore/ApplicationConfigurations/IApplication.hpp"
#include "EngineCore/IUpdatableSystem.hpp"
#include "EngineCore/EngineUpdatePhase.hpp"
#ifdef _WIN32
#include <Windows.h>
#ifdef ERROR
#undef ERROR
#endif // ERROR
#endif // _WIN32

using StateEngine::EngineCore::ApplicationConfigurations::IApplication;
using StateEngine::EngineCore::ApplicationConfigurations::EngineConfiguration;
namespace StateEngine::EngineCore {
	class EngineApplication {
	private:
		struct LoadedModuleContext {
			IModule* instance;
#ifdef _WIN32
			HMODULE windowsHandle;
#endif
		};
		static ZBuffer<LoadedModuleContext> loadedModules_;
		static ZBuffer<IEngineSystem*> inputPhaseSystems_;
		static ZBuffer<IEngineSystem*> preLogicPhaseSystems_;
		static ZBuffer<IEngineSystem*> physicsPhaseSystems_;
		static ZBuffer<IEngineSystem*> postLogicPhaseSystems_;
		static ZBuffer<IEngineSystem*> renderPhaseSystems_;
	public:
		static void Run(IApplication* app);
		static void RegisterEngineSystem(EngineUpdatePhase phase, IEngineSystem* system);
	private:
		static IModule* loadModule(const ZString& path);
		static void applyConfigurations(IModule* module, const EngineConfiguration& configuration);
	};
}