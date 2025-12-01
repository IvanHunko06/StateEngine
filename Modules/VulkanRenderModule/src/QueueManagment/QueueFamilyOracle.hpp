#pragma once
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistryMacros.hpp"
#include "Volk/volk.h"
#include <optional>

using StateEngine::EngineCore::DataStructures::ZBuffer;

namespace QueueManagment {
	class QueueFamilyOracle {
	public:
		inline static std::optional<uint32_t> findQueueFamily(const ZBuffer<VkQueueFamilyProperties>& families, VkQueueFlags requiredFlags) {
			for (uint32_t i = 0; i < families.size(); ++i) {
				if ((families[i].queueFlags & requiredFlags) == requiredFlags) {
					return i;
				}
			}
			return std::nullopt;
		}
		inline static std::optional<uint32_t> findQueueFamily(const ZBuffer<VkQueueFamilyProperties>& families, VkQueueFlags requiredFlags, VkQueueFlags unwantedFlags) {
			for (uint32_t i = 0; i < families.size(); ++i) {
				bool hasRequired = (families[i].queueFlags & requiredFlags) == requiredFlags;
				bool hasUnwanted = (families[i].queueFlags & unwantedFlags) != 0;

				if (hasRequired && !hasUnwanted) {
					return i;
				}
			}
			return std::nullopt;
		}
		inline static std::optional<uint32_t> findPresentFamily(const ZBuffer<VkQueueFamilyProperties>& families, const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {
			const TypeInfo* vkResultTypeInfo = GET_TYPE_INFO("VulkanRenderModule::VkResult");
			for (uint32_t i = 0; i < families.size(); ++i) {
				VkBool32 supported;
				VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported);
				if (result != VkResult::VK_SUCCESS) {
					if (vkResultTypeInfo)
						ZLOG_ERROR("VulkanRenderModule") << "Failed to get queue " << i << " surface support";
				}
				if (supported == VK_TRUE) {
					return i;
				}
			}
			return std::nullopt;
		}
	};
}