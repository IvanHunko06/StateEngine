#pragma once
#include "concurrentqueue.h"
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"

namespace StateEngine::EngineCore::Threading {
	struct MoodycamelQueueTraits : public moodycamel::ConcurrentQueueDefaultTraits {
		static inline void* malloc(size_t size) {
			return MemoryAllocator_AlignedAllocate(size, alignof(std::max_align_t));
		}
		static inline void free(void* ptr) {
			MemoryAllocator_Deallocate(ptr);
		}
	};

	template<typename TItem>
	using MPMCQueue = moodycamel::ConcurrentQueue<TItem, MoodycamelQueueTraits>;
}