#pragma once
#include "volk.h"
#include "VulkanPhysicalDevice.hpp"
#include "ZVulkanInfo.hpp"
#include "ZSwapchainManager.hpp"
#include "FrameManagment/ZFrameManager.hpp"

using FrameManagment::ZFrameManager;
class ZVulkanRenderDevice {
private:
	bool initialized_{ false };
	const ZVulkanInfo* vulkanInfo_{ nullptr };
	const TypeInfo* vkResultTypeInfo_{ nullptr };
	const VulkanPhysicalDevice* activePhysicalDeviceDesc_{ nullptr };
	const VkAllocationCallbacks* allocationCallbacks_{ nullptr };
	VkDevice logicalDevice_{ VK_NULL_HANDLE };
	
	friend class ZSwapchainManager;
	ZSwapchainManager swapchainManager_{};

	friend class ZFrameManager;
	//ZFrameManager frameManager_{};


public:
	ZVulkanRenderDevice() = default;
	ZVulkanRenderDevice(const ZVulkanRenderDevice&) = delete;
	ZVulkanRenderDevice& operator=(const ZVulkanRenderDevice&) = delete;

	bool initialize(const ZVulkanInfo& vulkanInfo);
	void shutdown();

	VkFence createFence(const char* fenceName);
	VkSemaphore createSemaphore(const char* semaphoreName);

private:
	bool createLogicalDevice();
	ZBuffer<VkDeviceQueueCreateInfo> collectDeviceQueueCreateInfo(ZBuffer<float>& allPriorities);
};