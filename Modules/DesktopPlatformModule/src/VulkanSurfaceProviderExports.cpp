#include "DesktopPlatformModule/DesktopPlatformModuleAPI.hpp"
#include "DesktopPlatformModule/DesktopPlatformModule.hpp"
#include "EngineCore/DataStructures/ZString.hpp"
#include "SDL3/SDL_vulkan.h"
#include "SDL3/SDL_log.h"
#include <cstdint>
#include <thread>

using StateEngine::EngineCore::DataStructures::ZString;
using StateEngine::DesktopPlatformModule::DesktopPlatformModule;
SDL_Window* safelyGetWindow() {
	constexpr auto kWindowWaitTimeout = std::chrono::seconds(5);
	DesktopPlatformModule& instance = DesktopPlatformModule::getInstance();
	SDL_Window* sdlWindow = nullptr;
	auto startTime = std::chrono::steady_clock::now();
	while (!sdlWindow) {
		sdlWindow = instance.getGameWindow().getSdlWindow();
		auto elapsedTime = std::chrono::steady_clock::now() - startTime;
		if (elapsedTime > kWindowWaitTimeout) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Timed out waiting for SDL window!");
			return nullptr;
		}

		if (!sdlWindow) std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	return sdlWindow;
}

extern "C" {
	DESKTOP_PLATFORM_MODULE_API void Vulkan_GetSurfaceExtensions(uint32_t* extensionsCount, ZString* names) {
		*extensionsCount = 0;

		const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(extensionsCount);
		if (!extensions || !*extensionsCount) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL get vulkan instance extensions failed: %s", SDL_GetError());
			return;
		}
		if (!names) return;

		for (uint32_t i = 0; i < *extensionsCount; ++i) {
			const char* curExtension = extensions[i];
			names[i] = curExtension;
		}
	}
	DESKTOP_PLATFORM_MODULE_API void Vulkan_CreateSurface(void* vkInstance, const void* vkAllocationCallbacks, uint64_t* vkSurface) {
		SDL_Window* sdlWindow = safelyGetWindow();
		if (!sdlWindow) return;

		if (!SDL_Vulkan_CreateSurface(
			sdlWindow, 
			static_cast<VkInstance>(vkInstance), 
			static_cast<const VkAllocationCallbacks*>(vkAllocationCallbacks), 
			reinterpret_cast<VkSurfaceKHR*>(vkSurface)))
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL create surface failed: %s", SDL_GetError());
			*vkSurface = 0;
		}
	}
}