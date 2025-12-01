#pragma once
#include "VulkanRenderModuleAPI.hpp"
#include "EngineCore/IModule.hpp"

using StateEngine::EngineCore::IModule;
extern "C" {
	VULKAN_RENDER_MODULE_API IModule* CreateModule();
}