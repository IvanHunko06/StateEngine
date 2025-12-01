#include "ZSwapchainManager.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistryMacros.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "ZVulkanRenderDevice.hpp"
using namespace StateEngine::EngineCore::EngineTypeSystem;

bool ZSwapchainManager::initialize(const ZVulkanRenderDevice& renderDevice) {
	
	logicalDevice_ = renderDevice.logicalDevice_;
	physicalDeviceDesc_ = renderDevice.activePhysicalDeviceDesc_;
	surface_ = renderDevice.vulkanInfo_->getSurface();
	allocationCallbacks_ = renderDevice.allocationCallbacks_;
	vkSurfaceFormatTypeInfo = GET_TYPE_INFO("VulkanRenderModule::VkSurfaceFormatKHR");
	vkPresentModeTypeInfo = GET_TYPE_INFO("VulkanRenderModule::VkPresentModeKHR");

	if (!vkSurfaceFormatTypeInfo) {
		ZLOG_ERROR("VulkanRenderModule") << "Failed to get required type VkSurfaceFormatKHR info";
		return false;
	}
	if (!vkPresentModeTypeInfo) {
		ZLOG_ERROR("VulkanRenderModule") << "Failed to get required type VkPresentModeKHR info";
		return false;
	}

	const VkSurfaceCapabilitiesKHR& surfaceCapabilities = physicalDeviceDesc_->surfaceCapabilities;
	imagesCount_ = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount && imagesCount_ > surfaceCapabilities.maxImageCount)
		imagesCount_ = surfaceCapabilities.maxImageCount;
	
	selectSurfaceFormat();
	selectPresentMode();

	ZLOG_DEBUG("VulkanRenderModule") << "Swapchain configuration: ";
	ZLOG_DEBUG("VulkanRenderModule") << " > Images count: " << imagesCount_;
	ZLOG_DEBUG("VulkanRenderModule") << " > Selected Surface Format: " << vkSurfaceFormatTypeInfo->toString(TypeInstance(&selectedSurfaceFormat_, vkSurfaceFormatTypeInfo), nullptr).c_str();
	ZLOG_DEBUG("VulkanRenderModule") << " > Selected Present Mode: " << vkPresentModeTypeInfo->toString(TypeInstance(&selectedPresentMode_, vkPresentModeTypeInfo), nullptr).c_str();
	VkSwapchainCreateInfoKHR swapchainCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = surface_,
		.minImageCount = imagesCount_,
		.imageFormat = selectedSurfaceFormat_.format,
		.imageColorSpace = selectedSurfaceFormat_.colorSpace,
		.imageExtent = surfaceCapabilities.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.preTransform = surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = selectedPresentMode_,
		.clipped = VK_TRUE,
		.oldSwapchain = nullptr
	};

	VkResult result = vkCreateSwapchainKHR(logicalDevice_, &swapchainCreateInfo, allocationCallbacks_, &swaptchain_);
	if (result != VK_SUCCESS) {
		ZLOG_ERROR("VulkanRenderModule") << "Failed to create swapchain: " << renderDevice.vkResultTypeInfo_->toString(TypeInstance(&result, renderDevice.vkResultTypeInfo_), nullptr).c_str();
		return false;
	}
	return true;
}
void ZSwapchainManager::shutdown() {
	if (!logicalDevice_) return;

	vkDeviceWaitIdle(logicalDevice_);
	if (swaptchain_) {
		vkDestroySwapchainKHR(logicalDevice_, swaptchain_, allocationCallbacks_);
		swaptchain_ = VK_NULL_HANDLE;
	}
}

void ZSwapchainManager::selectSurfaceFormat() {
	const ZBuffer<VkSurfaceFormatKHR>& surfaceFormats = physicalDeviceDesc_->surfaceFormats;

	for (size_t i = 0; i < surfaceFormats.size(); ++i) {
		const VkSurfaceFormatKHR& surfaceFormat = surfaceFormats[i];
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			selectedSurfaceFormat_ = surfaceFormat;
			return;
		}
	}

	if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
		selectedSurfaceFormat_ ={ VK_FORMAT_B8G8R8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
		return;
	}

	int32_t fallbackSrgbFormat = -1;
	int32_t fallbackUnormBGRAFormat = -1;
	int32_t fallbackUnormRGBAFormat = -1;

	for (size_t i = 0; i < surfaceFormats.size(); ++i) {
		const VkSurfaceFormatKHR& surfaceFormat = surfaceFormats[i];

		if (fallbackSrgbFormat == -1 &&
			surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
			surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			fallbackSrgbFormat = (int32_t)i;
		}
		if (fallbackUnormBGRAFormat == -1 &&
			surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
			surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			fallbackUnormBGRAFormat = (int32_t)i;
		}
		if (fallbackUnormRGBAFormat == -1 &&
			surfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM &&
			surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			fallbackUnormRGBAFormat = (int32_t)i;
		}
	}

	if (fallbackSrgbFormat != -1)
		selectedSurfaceFormat_ = surfaceFormats[fallbackSrgbFormat];
	else if (fallbackUnormBGRAFormat != -1)
		selectedSurfaceFormat_ = surfaceFormats[fallbackUnormBGRAFormat];
	else if (fallbackUnormRGBAFormat != -1)
		selectedSurfaceFormat_ = surfaceFormats[fallbackUnormRGBAFormat];
	else
		selectedSurfaceFormat_ = surfaceFormats[0];
}

void ZSwapchainManager::selectPresentMode() {
	ZBuffer<VkPresentModeKHR> supportedModes = physicalDeviceDesc_->presentModes;
	for (auto& presentMode : supportedModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			selectedPresentMode_ = presentMode;
			return;
		}
	}
	selectedPresentMode_ = VK_PRESENT_MODE_FIFO_KHR;
}