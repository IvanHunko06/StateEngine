#pragma once
#include "volk.h"
#include "ZFrameData.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
class ZVulkanRenderDevice;
using StateEngine::EngineCore::DataStructures::ZBuffer;
using StateEngine::EngineCore::EngineTypeSystem::TypeInfo;

namespace FrameManagment {
	class ZFrameManager {
	private:
		VkDevice device_{ VK_NULL_HANDLE };
		uint32_t currentFrameIndex_ = 0;
		uint32_t maxFramesInFlight_ = 2;
		ZBuffer<ZFrameData> frames_{};
		uint32_t workerThreadsCount_ = 8;
		const TypeInfo* vkResultTypeInfo_{ nullptr };
	public:
		bool initialize(const ZVulkanRenderDevice& renderDevice, uint32_t workerThreadsCount);
		void shutdown();
	};
}