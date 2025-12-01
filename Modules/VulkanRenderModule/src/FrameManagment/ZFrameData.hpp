#pragma once
#include "CommandManagment/ZCommandPool.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "volk.h"

using CommandManagment::ZCommandPool;
using StateEngine::EngineCore::DataStructures::ZBuffer;

struct ZFrameData {
	struct ZThreadResources {
		ZCommandPool graphicsPool;
		ZCommandPool computePool;
		ZCommandPool transferPool;
	};

	VkFence inFlightFence{ VK_NULL_HANDLE };
	VkSemaphore imageAvailableSemaphore{ VK_NULL_HANDLE };
	VkSemaphore renderFinishedSemaphore{ VK_NULL_HANDLE };

	ZBuffer<ZThreadResources> threadResources{};
};