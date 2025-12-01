#pragma once
#include "volk.h"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "VulkanPhysicalDevice.hpp"
#include "VulkanRenderModule/Configurations/IVulkanSurfaceProvider.hpp"
#include "VulkanRenderModule/Configurations/VulkanCreateInstanceConfiguration.hpp"
#include "VulkanRenderModule/Configurations/OverrideActiveVulkanDevice.hpp"

using StateEngine::EngineCore::EngineTypeSystem::TypeInfo;
using namespace StateEngine::EngineCore::DataStructures;

class ZVulkanInfo {
private:
	IVulkanSurfaceProvider* vulkanSurfaceProvider_{ nullptr };
	VulkanCreateInstanceConfiguration* vulkanCreateInstanceConfiguration_{ nullptr };
	OverrideActiveVulkanDevice* overrideActiveVulkanDevice{ nullptr };
	const TypeInfo* vkResultTypeInfo_{ nullptr };
	VkAllocationCallbacks allocationCallbacks_{};
	VkInstance instance_{ VK_NULL_HANDLE };
	VkSurfaceKHR surface_{ VK_NULL_HANDLE };
	ZBuffer<VulkanPhysicalDevice> physicalDevices_;
public:
	bool initialize();
	void shutdown();
	const VulkanPhysicalDevice& selectActivePhysicalDevice() const noexcept;
	inline const VkInstance& getInstance() const noexcept {
		return instance_;
	}
	inline const VkAllocationCallbacks& getAllocationCallbacks() const noexcept {
		return allocationCallbacks_;
	}
	inline const VkSurfaceKHR& getSurface() const noexcept {
		return surface_;
	}

private:
	bool initializeServices();
	bool initializeVolk();
	void setupAllocationCallbacks();
#ifdef _WIN32
	void warmUpAllDevices();
	ZString wideToUTF8(const wchar_t* wideString);
#endif
	bool createVulkanInstance();
	bool enumeratePhysicalDevices();
	
};