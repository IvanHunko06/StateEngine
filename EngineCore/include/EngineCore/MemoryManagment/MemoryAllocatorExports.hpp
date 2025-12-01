#pragma once
#include "EngineCore/EngineCoreAPI.hpp"

extern "C" {
	ENGINE_CORE_API void* MemoryAllocator_AlignedAllocate(size_t size, size_t alignment);
	ENGINE_CORE_API void  MemoryAllocator_Deallocate(void* ptr);
	ENGINE_CORE_API void* MemoryAllocator_Calloc(size_t nmemb, size_t size, size_t alignment);
	ENGINE_CORE_API void* MemoryAllocator_Realloc(void* ptr, size_t new_size, size_t alignment);
}