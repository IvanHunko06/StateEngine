#pragma once
#include "blockingconcurrentqueue.h"
#include "MoodycamelQueueTraits.hpp"
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"

namespace StateEngine::EngineCore::Threading {
	template<typename TItem>
	using BlockingMPMCQueue = moodycamel::BlockingConcurrentQueue<TItem, MoodycamelQueueTraits>;
}