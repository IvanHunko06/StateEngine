#pragma once
#include "EngineCore/DataStructures/ZString.hpp"

using StateEngine::EngineCore::DataStructures::ZString;
class IVulkanSurfaceProvider {
public:
	virtual void getSurfaceExtensions(uint32_t* extensionsCount, ZString* names) = 0;
	virtual void createSurface(void* vkInstance, const void* vkAllocationCallbacks, uint64_t* vkSurface) = 0;
};