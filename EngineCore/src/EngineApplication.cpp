#include "EngineApplication.hpp"
#include "EngineCore/BaseEngineEvents/ShutdownEngineEvent.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "Logging/ZLogger.hpp"
#include "EngineCore/ConfigurationConsumers/IWindowConfigurationConsumer.hpp"
#include "EngineTypeSystem/ZTypeRegistry.hpp"
#include "EventBus/ZEventBus.hpp"
#include "ServiceLocator/ZServiceLocator.hpp"
#include "Threading/CpuCoresBinding.hpp"
#include "JobSystem/ZJobSystem.hpp"
#include "EngineCore/EventBus/EventBusWrapper.hpp"
#include <cassert>
#include <chrono>
#include <atomic>

using StateEngine::EngineCore::EngineApplication;
using StateEngine::EngineCore::IModule;
using StateEngine::EngineCore::IEngineSystem;
using StateEngine::EngineCore::EventBus::ZEventBus;
using StateEngine::EngineCore::EngineTypeSystem::ZTypeRegistry;
using StateEngine::EngineCore::Logging::ZLogger;
using StateEngine::EngineCore::Threading::CpuCoresBinding;
using StateEngine::EngineCore::ServiceLocator::ZServiceLocator;
using StateEngine::EngineCore::JobSystem::ZJobSystem;
using namespace StateEngine::EngineCore::ApplicationConfigurations;
using namespace StateEngine::EngineCore::ApplicationConfigurations::Consumers;
using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::EngineCore::EventBus;
using namespace StateEngine::EngineCore::BaseEngineEvents;

ZBuffer<EngineApplication::LoadedModuleContext> EngineApplication::LoadedModules;
ZBuffer<IEngineSystem*> EngineApplication::InputPhaseSystems;
ZBuffer<IEngineSystem*> EngineApplication::RenderPhaseSystems;
ZEventBus EngineApplication::GlobalEventBus = ZEventBus(2 * 1024 * 1024);

void EngineApplication::Run(IApplication* app) {
    ZTypeRegistry::RegisterBaseTypes();
    ZEventBus::RegisterBaseEventsTypes();

	// 1. Configuration
	EngineBuilder builder;
	app->configureEngine(builder);

	EngineConfiguration config = builder.build();
	ZLogger::SetSinksAndLevelsSealed(true);
#ifdef _WIN32
	CpuCoresBinding::SetUseCpuSets(config.useCpuSets);
#endif
	CpuCoresBinding::CollectCores();
	CpuCoresBinding::BindThreadToCore({ CpuCoresBinding::GetMainCoreId().logicalId });
	ZJobSystem::Initialize();
	
	// 2. Load modules
	for (auto& path : config.modulesToLoad) {
		ZLOG_DEBUG("EngineCore") << "loading module: " << path.c_str();
		IModule* instance = loadModule(path);
		if (instance == nullptr) {
			ZLOG_WARN("EngineCore") << "failed to load module: " << path.c_str();
			continue;
		}
		instance->RegisterTypes();
		ZLOG_DEBUG("EngineCore") << "registered types from module: " << instance->GetName();
	}
    ZTypeRegistry::SetIsSealed(true);
	

	if (config.dependencyRegistrationCallback) {
		ZLOG_DEBUG("EngineCore") << "Calling DependencyRegistrationCallback";
		config.dependencyRegistrationCallback();
	}
	else {
		ZLOG_DEBUG("EngineCore") << "Skipping DependencyRegistrationCallback";
	}
	ZServiceLocator::SetIsSealed(true);

	for (auto& loadedModule : LoadedModules) {
		loadedModule.instance->OnLoad();
		ZLOG_DEBUG("EngineCore") << "module loaded: " << loadedModule.instance->GetName();
	}
	ZLogger::FlushMessages();

	// 3. Main loop
	bool isRunning{ true };
	auto lastTime = std::chrono::high_resolution_clock::now();

    EventBusWrapper::Subscribe<ShutdownEngineEvent>([&isRunning](const ShutdownEngineEvent* eventData) {
        isRunning = false;
        return false;
    });
	while (isRunning){
		auto curentTime = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<float> duration = curentTime - lastTime;
		const float deltaTime = duration.count();
		lastTime = curentTime;

		for (auto& system : InputPhaseSystems) {
			system->UpdateSystem(deltaTime);
		}
        GlobalEventBus.FlushEvents();
		if (!isRunning) {
            break;
		}

		for (auto& system : RenderPhaseSystems) {
			system->UpdateSystem(deltaTime);
		}
		ZLogger::FlushMessages();
	}

	// 4. Unload modules
    ZTypeRegistry::SetIsSealed(false);
	for (auto& engineModuleContext : LoadedModules) {
		engineModuleContext.instance->UnregisterTypes();
		engineModuleContext.instance->OnUnload();
#ifdef _WIN32
		FreeLibrary(engineModuleContext.windowsHandle);
#endif
	}

	// 5. Call onShutdown event for application
	app->onShutdown();
	
	ZJobSystem::Shutdown();
    ZEventBus::UnregisterBaseEventsTypes();
    ZTypeRegistry::UnregisterBaseTypes();
    ZTypeRegistry::ÑheckAllTypesRelease();
}

IModule* EngineApplication::loadModule(const ZString& path) {
	using CreateModuleFunc = IModule * (*)();
	CreateModuleFunc createFunc = nullptr;
	LoadedModuleContext context;
#ifdef _WIN32
	HMODULE moduleHandle = LoadLibraryA(path.c_str());
	if (moduleHandle == NULL) {
		assert(moduleHandle && "Failed to load module");
		return nullptr;
	}
	createFunc = reinterpret_cast<CreateModuleFunc>(GetProcAddress(moduleHandle, "CreateModule"));
	if (createFunc == NULL) {
		assert(createFunc && "CreateModule function not found in loaded module");
		FreeLibrary(moduleHandle);
		return nullptr;
	}
	context.windowsHandle = moduleHandle;
#endif

	if (createFunc == nullptr) return nullptr;

	context.instance = createFunc();
	if (context.instance == nullptr) {
		assert(context.instance && "Failed to create module instance");
#ifdef _WIN32
		FreeLibrary(context.windowsHandle);
#endif
		return nullptr;
	}

	LoadedModules.push_back(context);

	return context.instance;
}


void EngineApplication::RegisterEngineSystem(EngineUpdatePhase phase, IEngineSystem* system) {
	switch (phase)
	{
	case StateEngine::EngineCore::EngineUpdatePhase::Input:
		InputPhaseSystems.push_back(system);
		break;
	case StateEngine::EngineCore::EngineUpdatePhase::Render:
		RenderPhaseSystems.push_back(system);
		break;
	default:
		break;
	}
}