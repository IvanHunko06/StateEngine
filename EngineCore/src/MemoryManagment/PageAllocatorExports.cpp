#include "EngineCore/EngineCoreAPI.hpp"
#include "MemoryManagment/PageAllocator.hpp"
using namespace StateEngine::EngineCore::MemoryManagment;
extern "C" {
	ENGINE_CORE_API void* PageAllocator_ReserveMemory(size_t size) {
		return PageAllocator::reserve(size);
	}

	ENGINE_CORE_API bool PageAllocator_TryCommitMemory(void* addr, size_t size) {
		return PageAllocator::tryCommit(addr, size);
	}

	ENGINE_CORE_API bool PageAllocator_TryReleaseMemory(void* addr) {
		return PageAllocator::tryRelease(addr);
	}

	ENGINE_CORE_API size_t PageAllocator_GetPageSize() {
		return PageAllocator::getPageSize();
	}
}