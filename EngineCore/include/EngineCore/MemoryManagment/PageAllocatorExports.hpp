#pragma once
#include "EngineCore/EngineCoreAPI.hpp"

extern "C" {
	ENGINE_CORE_API void* PageAllocator_ReserveMemory(size_t size);

	ENGINE_CORE_API bool PageAllocator_TryCommitMemory(void* addr, size_t size);

	ENGINE_CORE_API bool PageAllocator_TryReleaseMemory(void* addr);

	ENGINE_CORE_API size_t PageAllocator_GetPageSize();
}