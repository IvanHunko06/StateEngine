#pragma once
#include "volk.h"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/DataStructures/ZHashSet.hpp"
#include "EngineCore/DataStructures/ZString.hpp"

using namespace StateEngine::EngineCore::DataStructures;

struct VulkanPhysicalDevice {
	uint32_t deviceIndex{ 0 };
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
	VkPhysicalDeviceProperties properties{};
	ZBuffer<VkQueueFamilyProperties> queueFamilyProperties{};
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	VkPhysicalDeviceFeatures supportedFeatures{};
	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	ZBuffer<VkSurfaceFormatKHR> surfaceFormats{};
	ZBuffer<VkPresentModeKHR> presentModes{};
	ZHashSet<ZString> supportedExtensions{};
};