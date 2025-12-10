#include "ZVulkanRenderDevice.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "QueueManagment/QueueFamilyOracle.hpp"
#include "EngineCore/DataStructures/ZHashSet.hpp"
#include <assert.h>

using namespace QueueManagment;
using namespace StateEngine::EngineCore::DataStructures;
using namespace StateEngine::EngineCore::EngineTypeSystem;

bool ZVulkanRenderDevice::initialize(const ZVulkanInfo& vulkanInfo) {
	vulkanInfo_ = &vulkanInfo;
	activePhysicalDeviceDesc_ = &vulkanInfo.selectActivePhysicalDevice();
	ZLOG_INFO("VulkanRenderModule") << "Active Vulkan Device: " << activePhysicalDeviceDesc_->deviceIndex;
	allocationCallbacks_ = &vulkanInfo.getAllocationCallbacks();
	vkResultTypeInfo_ = GET_TYPE_INFO("VulkanRenderModule::VkResult");

	if (!createLogicalDevice())
		return false;

	//frameManager_.initialize(*this);

	if(!swapchainManager_.initialize(*this))
		return false;

	return true;
}
void ZVulkanRenderDevice::shutdown() {
	if (!logicalDevice_) return;
	vkDeviceWaitIdle(logicalDevice_);
	swapchainManager_.shutdown();
	vkDestroyDevice(logicalDevice_, allocationCallbacks_);
}

bool ZVulkanRenderDevice::createLogicalDevice() {
	
	ZBuffer<float> allPriorities;
	ZBuffer<VkDeviceQueueCreateInfo> queueCreateInfos = collectDeviceQueueCreateInfo(allPriorities);
	if (queueCreateInfos.empty()) return false;

	VkPhysicalDeviceFeatures enabledFeatures{};

	ZBuffer<const char*> enabledExtensions{
		"VK_KHR_swapchain"
	};

	ZLOG_DEBUG("VulkanRenderModule") << "Enabling " << enabledExtensions.size()
		<< " device extensions...";
	for (auto& ext : enabledExtensions) {
		ZLOG_DEBUG("VulkanRenderModule") << "  > " << ext;
	}

	VkDeviceCreateInfo deviceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
		.ppEnabledExtensionNames = enabledExtensions.data(),
		.pEnabledFeatures = &enabledFeatures
	};

	VkResult result = vkCreateDevice(activePhysicalDeviceDesc_->physicalDevice, &deviceCreateInfo, allocationCallbacks_, &logicalDevice_);
	if (result != VK_SUCCESS) {
		if (vkResultTypeInfo_)
			ZLOG_ERROR("VulkanRenderModule") << "Failed to create vulkan device: " << vkResultTypeInfo_->toString(TypeInstance(&result, vkResultTypeInfo_), nullptr).c_str();
		
		return false;
	}
	volkLoadDevice(logicalDevice_);
	return true;
}

ZBuffer<VkDeviceQueueCreateInfo> ZVulkanRenderDevice::collectDeviceQueueCreateInfo(ZBuffer<float>& allPriorities) {
	ZBuffer<VkDeviceQueueCreateInfo> queueCreateInfos;
	const ZBuffer<VkQueueFamilyProperties> queueFamiliesProperties = activePhysicalDeviceDesc_->queueFamilyProperties;

	std::optional<uint32_t> graphicsFamily = QueueFamilyOracle::findQueueFamily(queueFamiliesProperties, VK_QUEUE_GRAPHICS_BIT);
	std::optional<uint32_t> computeFamily = QueueFamilyOracle::findQueueFamily(queueFamiliesProperties, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT);
	if (!computeFamily.has_value())
		computeFamily = QueueFamilyOracle::findQueueFamily(queueFamiliesProperties, VK_QUEUE_COMPUTE_BIT);
	std::optional<uint32_t> transferFamily = QueueFamilyOracle::findQueueFamily(queueFamiliesProperties, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
	if (!transferFamily.has_value())
		transferFamily = QueueFamilyOracle::findQueueFamily(queueFamiliesProperties, VK_QUEUE_TRANSFER_BIT);

	std::optional<uint32_t> presentFamily = QueueFamilyOracle::findPresentFamily(queueFamiliesProperties, activePhysicalDeviceDesc_->physicalDevice, vulkanInfo_->getSurface());

	assert(graphicsFamily.has_value() && "No Graphics Family found!");
	assert(computeFamily.has_value() && "No Compute Family found!");
	assert(transferFamily.has_value() && "No Transfer Family found!");
	assert(presentFamily.has_value() && "No Present Family found!");

	if (!graphicsFamily.has_value()) {
		ZLOG_ERROR("VulkanRenderModule") << "No Graphics Family found for device " << activePhysicalDeviceDesc_->deviceIndex;
		return queueCreateInfos;
	}
	else {
		ZLOG_DEBUG("VulkanRenderModule") << "Graphics Family index " << graphicsFamily.value() << " for device " << activePhysicalDeviceDesc_->deviceIndex;
	}

	if (!computeFamily.has_value()) {
		ZLOG_ERROR("VulkanRenderModule") << "No Compute Family found for device " << activePhysicalDeviceDesc_->deviceIndex;
		return queueCreateInfos;
	}
	else {
		ZLOG_DEBUG("VulkanRenderModule") << "Compute Family index " << computeFamily.value() << " for device " << activePhysicalDeviceDesc_->deviceIndex;
	}

	if (!transferFamily.has_value()) {
		ZLOG_ERROR("VulkanRenderModule") << "No Transfer Family found for device " << activePhysicalDeviceDesc_->deviceIndex;
		return queueCreateInfos;
	}
	else {
		ZLOG_DEBUG("VulkanRenderModule") << "Transfer Family index " << transferFamily.value() << " for device " << activePhysicalDeviceDesc_->deviceIndex;
	}

	if (!presentFamily.has_value()) {
		ZLOG_ERROR("VulkanRenderModule") << "No Present Family found for device " << activePhysicalDeviceDesc_->deviceIndex;
		return queueCreateInfos;
	}
	else {
		ZLOG_DEBUG("VulkanRenderModule") << "Present Family index " << presentFamily.value() << " for device " << activePhysicalDeviceDesc_->deviceIndex;
	}


	ZHashSet<uint32_t> uniqueFamilies;
	uniqueFamilies.insert(graphicsFamily.value());
	uniqueFamilies.insert(computeFamily.value());
	uniqueFamilies.insert(transferFamily.value());
	uniqueFamilies.insert(presentFamily.value());

	for (auto queueFamilyIndex : uniqueFamilies) {
		allPriorities.push_back(1.0f);
		queueCreateInfos.push_back(VkDeviceQueueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueFamilyIndex = queueFamilyIndex,
			.queueCount = 1,
			.pQueuePriorities = &allPriorities[allPriorities.size() - 1]
		});
	}
	return queueCreateInfos;
}

VkFence ZVulkanRenderDevice::createFence(const char* fenceName) {
	if (!logicalDevice_) return VK_NULL_HANDLE;
	VkFenceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0
	};
	VkFence fence;
	VkResult res = vkCreateFence(logicalDevice_, &createInfo, allocationCallbacks_, &fence);
	if (res != VK_SUCCESS) {
		ZLOG_WARN("VulkanRenderModule") << "Failed to create fence '" << fenceName << "': " << vkResultTypeInfo_->toString(TypeInstance(&res, vkResultTypeInfo_), nullptr).c_str();
		return VK_NULL_HANDLE;
	}
#ifdef _DEBUG
	ZLOG_DEBUG("VulkanRenderModule") << "Created fence: '" << fenceName << "'";
	VkDebugUtilsObjectNameInfoEXT objecctName{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.pNext = nullptr,
		.objectType = VK_OBJECT_TYPE_FENCE,
		.objectHandle = reinterpret_cast<uint64_t>(fence),
		.pObjectName = fenceName
	};
	res = vkSetDebugUtilsObjectNameEXT(logicalDevice_, &objecctName);
	if (res != VK_SUCCESS) {
		ZLOG_WARN("VulkanRenderModule") << "Failed to set object name '" << fenceName << "': " << vkResultTypeInfo_->toString(TypeInstance(&res, vkResultTypeInfo_), nullptr).c_str();
	}
#endif
	return fence;
}
VkSemaphore ZVulkanRenderDevice::createSemaphore(const char* semaphoreName) {
	if (!logicalDevice_) return VK_NULL_HANDLE;
	VkSemaphoreCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0
	};
	VkSemaphore semaphore;
	VkResult res = vkCreateSemaphore(logicalDevice_, &createInfo, allocationCallbacks_, &semaphore);
	if (res != VK_SUCCESS) {
		ZLOG_WARN("VulkanRenderModule") << "Failed to create semaphore '" << semaphoreName << "': " << vkResultTypeInfo_->toString(TypeInstance(&res, vkResultTypeInfo_), nullptr).c_str();
		return VK_NULL_HANDLE;
	}
#ifdef _DEBUG
	ZLOG_DEBUG("VulkanRenderModule") << "Created semaphore: '" << semaphoreName << "'";
	VkDebugUtilsObjectNameInfoEXT objecctName{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.pNext = nullptr,
		.objectType = VK_OBJECT_TYPE_SEMAPHORE,
		.objectHandle = reinterpret_cast<uint64_t>(semaphore),
		.pObjectName = semaphoreName
	};
	res = vkSetDebugUtilsObjectNameEXT(logicalDevice_, &objecctName);
	if (res != VK_SUCCESS) {
		ZLOG_WARN("VulkanRenderModule") << "Failed to set object name '" << semaphoreName << "': " << vkResultTypeInfo_->toString(TypeInstance(&res, vkResultTypeInfo_), nullptr).c_str();
	}
#endif
	return semaphore;
}