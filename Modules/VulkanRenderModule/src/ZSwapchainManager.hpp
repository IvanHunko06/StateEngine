#pragma once
#include "volk.h"
#include "VulkanPhysicalDevice.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
class ZVulkanRenderDevice;
using StateEngine::EngineCore::EngineTypeSystem::TypeInfo;

class ZSwapchainManager {
private:
	VkSwapchainKHR swaptchain_{ VK_NULL_HANDLE };
	VkSurfaceKHR surface_{ VK_NULL_HANDLE };
	VkDevice logicalDevice_{ VK_NULL_HANDLE };
	const VulkanPhysicalDevice* physicalDeviceDesc_{ nullptr };
	const VkAllocationCallbacks* allocationCallbacks_{ nullptr };
	uint32_t imagesCount_{ 0 };
	VkSurfaceFormatKHR selectedSurfaceFormat_{};
	const TypeInfo* vkSurfaceFormatTypeInfo{ nullptr };
	VkPresentModeKHR selectedPresentMode_{ VK_PRESENT_MODE_FIFO_KHR };
	const TypeInfo* vkPresentModeTypeInfo{ nullptr };
public:
	bool initialize(const ZVulkanRenderDevice& renderDevice);
	void shutdown();
	inline uint32_t getImagesCount() const noexcept {
		return imagesCount_;
	}
private:
	void selectSurfaceFormat();
	void selectPresentMode();
};