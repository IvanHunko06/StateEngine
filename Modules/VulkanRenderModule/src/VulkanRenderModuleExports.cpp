#include "VulkanRenderModule/VulkanRenderModuleExports.hpp"
#include "VulkanRenderModule/VulkanRenderModule.hpp"

extern "C" {
	VULKAN_RENDER_MODULE_API IModule* CreateModule() {
		static StateEngine::VulkanRenderModule::VulkanRenderModule instance;
		return &instance;
	}
}