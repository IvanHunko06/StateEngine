#include "DesktopPlatformModule/DesktopPlatformModule.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistryExports.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/ApplicationExports.hpp"
#include "SDL3/SDL.h"

using StateEngine::DesktopPlatformModule::DesktopPlatformModule;
using StateEngine::EngineCore::EngineUpdatePhase;
using namespace StateEngine::EngineCore::EngineTypeSystem;

static constexpr const char* getSdlLogCategoryName(SDL_LogCategory category) {
	switch (category)
	{
	case SDL_LOG_CATEGORY_APPLICATION:
		return "application";
	case SDL_LOG_CATEGORY_ERROR:
		return "error";
	case SDL_LOG_CATEGORY_ASSERT:
		return "assert";
	case SDL_LOG_CATEGORY_SYSTEM:
		return "system";
	case SDL_LOG_CATEGORY_AUDIO:
		return "audio";
	case SDL_LOG_CATEGORY_VIDEO:
		return "video";
	case SDL_LOG_CATEGORY_RENDER:
		return "render";
	case SDL_LOG_CATEGORY_INPUT:
		return "input";
	case SDL_LOG_CATEGORY_GPU:
		return "gpu";
	case SDL_LOG_CATEGORY_CUSTOM:
		return "custom";
	default:
		return "none";
	}
}

void DesktopPlatformModule::OnLoad() {
	SDL_SetMemoryFunctions(
		[](size_t sz) ->void* {
			return MemoryAllocator_AlignedAllocate(sz, 16);
		},
		[](size_t nmemb, size_t sz) -> void* {
			return MemoryAllocator_Calloc(nmemb, sz, 16);
		},
		[](void* mem, size_t sz) -> void* {
			return MemoryAllocator_Realloc(mem, sz, 16);
		},
		MemoryAllocator_Deallocate
	);

	SDL_SetLogOutputFunction(
		[](void* userdata, int category, SDL_LogPriority priority, const char* message) {
			switch (priority)
			{
			case SDL_LOG_PRIORITY_DEBUG:
				ZLOG_DEBUG("SDL3") << "[category:" << getSdlLogCategoryName(static_cast<SDL_LogCategory>(category)) << "]: " << message;
				break;
			case SDL_LOG_PRIORITY_INFO:
				ZLOG_INFO("SDL3") << "[category:" << getSdlLogCategoryName(static_cast<SDL_LogCategory>(category)) << "]: " << message;
				break;
			case SDL_LOG_PRIORITY_WARN:
				ZLOG_WARN("SDL3") << "[category:" << getSdlLogCategoryName(static_cast<SDL_LogCategory>(category)) << "]: " << message;
				break;
			case SDL_LOG_PRIORITY_ERROR:
				ZLOG_ERROR("SDL3") << "[category:" << getSdlLogCategoryName(static_cast<SDL_LogCategory>(category)) << "]: " << message;
				break;
			case SDL_LOG_PRIORITY_CRITICAL:
				ZLOG_FATAL("SDL3") << "[category:" << getSdlLogCategoryName(static_cast<SDL_LogCategory>(category)) << "]: " << message;
				break;
			default:
				break;
			}
		}, 
		nullptr);

	LogLevel sdlLogLevel = Logger_GetLogLevel("SDL3");
	switch (sdlLogLevel)
	{
	case StateEngine::EngineCore::Logging::LogLevel::DEBUG:
		SDL_SetLogPriorities(SDL_LOG_PRIORITY_DEBUG);
		break;
	case StateEngine::EngineCore::Logging::LogLevel::INFO:
		SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
		break;
	case StateEngine::EngineCore::Logging::LogLevel::WARNING:
		SDL_SetLogPriorities(SDL_LOG_PRIORITY_WARN);
		break;
	case StateEngine::EngineCore::Logging::LogLevel::ERROR:
		SDL_SetLogPriorities(SDL_LOG_PRIORITY_ERROR);
		break;
	case StateEngine::EngineCore::Logging::LogLevel::FATAL:
		SDL_SetLogPriorities(SDL_LOG_PRIORITY_CRITICAL);
		break;
	default:
		break;
	}

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL initialization failed: %s", SDL_GetError());
		return;
	}

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SDL initialized");
	Application_RegisterSystem(EngineUpdatePhase::Input, &inputEventsSystem_);
}
void DesktopPlatformModule::OnUnload() {
	gameWindow.destroy();
	SDL_Quit();
}
void DesktopPlatformModule::configureWindow(const WindowSettings& settings) {
	gameWindow.createWindow(settings);
}
void DesktopPlatformModule::RegisterTypes() {
}
void DesktopPlatformModule::UnregisterTypes() {
	
}

DesktopPlatformModule& DesktopPlatformModule::getInstance() {
	static DesktopPlatformModule instance;
	return instance;
}