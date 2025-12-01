#pragma once
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "ZCommandBuffer.hpp"
#include "volk.h"

using StateEngine::EngineCore::DataStructures::ZBuffer;
namespace CommandManagment {
	class ZCommandPool {
	private:
		static thread_local ZBuffer<VkCommandPool> familyCommandPools;
		uint32_t familyIndex;
	public:
		bool initialize(uint32_t familyInde);
		void shutdown();
		void reset();
		ZCommandBuffer getNewBuffer();
	};
}