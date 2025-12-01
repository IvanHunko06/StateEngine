#include "EngineCore/ApplicationExports.hpp"
#include "EngineApplication.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"

using StateEngine::EngineCore::EngineApplication;
extern "C" {
	ENGINE_CORE_API void Application_Run(IApplication* app) {
		static bool isRunning = false;
		if (isRunning) {
			ZLOG_WARN("EngineCore") << "Application_Run called while an application is already running. Ignoring.";
			return;
		}
		isRunning = true;
		EngineApplication::run(app);
		isRunning = false;
	}
	ENGINE_CORE_API void Application_RegisterSystem(EngineUpdatePhase phase, IUpdatableSystem* system) {
		EngineApplication::registerSystem(phase, system);
	}
}