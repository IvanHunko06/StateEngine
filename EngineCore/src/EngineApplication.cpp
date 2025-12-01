#include "EngineApplication.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "Logging/ZLogger.hpp"
#include "EngineCore/ConfigurationConsumers/IWindowConfigurationConsumer.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistryMacros.hpp"
#include "EngineTypeSystem/ZTypeRegistry.hpp"
#include "EventBus/ZEventBus.hpp"
#include "Threading/CpuCoresBinding.hpp"
#include <cassert>
#include <chrono>
#include <atomic>

using StateEngine::EngineCore::EngineApplication;
using StateEngine::EngineCore::IModule;
using StateEngine::EngineCore::IUpdatableSystem;
using StateEngine::EngineCore::EventBus::ZEventBus;
using StateEngine::EngineCore::EngineTypeSystem::ZTypeRegistry;
using StateEngine::EngineCore::Logging::ZLogger;
using StateEngine::EngineCore::Threading::CpuCoresBinding;
using namespace StateEngine::EngineCore::ApplicationConfigurations;
using namespace StateEngine::EngineCore::ApplicationConfigurations::Consumers;
using namespace StateEngine::EngineCore::EngineTypeSystem;

ZBuffer<EngineApplication::LoadedModuleContext> EngineApplication::loadedModules_;
ZBuffer<IUpdatableSystem*> EngineApplication::inputPhaseSystems_;
ZBuffer<IUpdatableSystem*> EngineApplication::preLogicPhaseSystems_;
ZBuffer<IUpdatableSystem*> EngineApplication::physicsPhaseSystems_;
ZBuffer<IUpdatableSystem*> EngineApplication::postLogicPhaseSystems_;
ZBuffer<IUpdatableSystem*> EngineApplication::renderPhaseSystems_;

void EngineApplication::run(IApplication* app) {
	// 1. Configuration
	EngineBuilder builder;
	app->configureEngine(builder);

	EngineConfiguration config = builder.build();
#ifdef _WIN32
	CpuCoresBinding::SetUseCpuSets(config.useCpuSets);
#endif
	CpuCoresBinding::CollectCores();

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
	ZTypeRegistry::SetRecordingAllowed(false);
	

	if (config.dependencyRegistrationCallback) {
		ZLOG_DEBUG("EngineCore") << "calling DependencyRegistrationCallback";
		config.dependencyRegistrationCallback(config.dependencyRegistrationCallbackUserData);
	}
	else {
		ZLOG_DEBUG("EngineCore") << "skipping DependencyRegistrationCallback";
	}

	for (auto& loadedModule : loadedModules_) {
		loadedModule.instance->OnLoad();
		applyConfigurations(loadedModule.instance, config);
		ZLOG_DEBUG("EngineCore") << "module loaded: " << loadedModule.instance->GetName();
	}
	ZLogger::FlushMessages();

	// 3. Main loop
	bool isRunning{ true };
	auto lastTime = std::chrono::high_resolution_clock::now();

	ZEventBus::subscribe(GET_TYPE_INFO("ShutdownEngineEvent"), &isRunning, [](void* listener, void* userData)->bool {
		bool* isRunningPtr = reinterpret_cast<bool*>(listener);
		*isRunningPtr = false;
		return true;
	});
	while (isRunning){
		auto curentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> duration = curentTime - lastTime;
		float deltaTime = duration.count();
		lastTime = curentTime;

		for (auto& system : inputPhaseSystems_) {
			system->UpdateSystem(deltaTime);
		}
		ZEventBus::flushEvents();
		if (!isRunning) break;

		for (auto& system : preLogicPhaseSystems_) {
			system->UpdateSystem(deltaTime);
		}
		for (auto& system : physicsPhaseSystems_) {
			system->UpdateSystem(deltaTime);
		}
		for (auto& system : postLogicPhaseSystems_) {
			system->UpdateSystem(deltaTime);
		}
		for (auto& system : renderPhaseSystems_) {
			system->UpdateSystem(deltaTime);
		}
		ZLogger::FlushMessages();
	}

	// 4. Unload modules
	ZTypeRegistry::SetRecordingAllowed(true);
	for (auto& engineModuleContext : loadedModules_) {
		engineModuleContext.instance->UnregisterTypes();
		engineModuleContext.instance->OnUnload();
#ifdef _WIN32
		FreeLibrary(engineModuleContext.windowsHandle);
#endif
	}
	ZTypeRegistry::SetRecordingAllowed(false);

	// 5. Call onShutdown event for application
	app->onShutdown();
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

	loadedModules_.push_back(context);

	return context.instance;
}

void EngineApplication::applyConfigurations(IModule* module, const EngineConfiguration& configuration) {
	if (IWindowConfigurationConsumer* windowConsumer = dynamic_cast<IWindowConfigurationConsumer*>(module)) {
		ZLOG_DEBUG("EngineCore") << "applying window configuration to module: " << module->GetName();
		windowConsumer->configureWindow(configuration.windowSettings);
	}
}

void EngineApplication::registerSystem(EngineUpdatePhase phase, IUpdatableSystem* system) {
	switch (phase)
	{
	case StateEngine::EngineCore::EngineUpdatePhase::Input:
		inputPhaseSystems_.push_back(system);
		break;
	case StateEngine::EngineCore::EngineUpdatePhase::PreLogic:
		preLogicPhaseSystems_.push_back(system);
		break;
	case StateEngine::EngineCore::EngineUpdatePhase::Physics:
		physicsPhaseSystems_.push_back(system);
		break;
	case StateEngine::EngineCore::EngineUpdatePhase::PostLogic:
		postLogicPhaseSystems_.push_back(system);
		break;
	case StateEngine::EngineCore::EngineUpdatePhase::Render:
		renderPhaseSystems_.push_back(system);
		break;
	default:
		break;
	}
}