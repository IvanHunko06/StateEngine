#include "EngineCore/EngineCoreAPI.hpp"
#include "MemoryManagment/ZAllocator_SEMalloc_Virtual.hpp"
#ifdef USE_MIMALLOC_ALLOCATOR
#include "mimalloc.h"
#endif


using namespace StateEngine::EngineCore::MemoryManagment;

extern "C" {
	ENGINE_CORE_API void* MemoryAllocator_AlignedAllocate(size_t size, size_t alignment) {
#ifdef USE_MIMALLOC_ALLOCATOR
		return mi_aligned_alloc(alignment, size);
#else
		return ZAllocator_SEMalloc_Virtual::alignedAllocate(size, alignment);
#endif
	}
	ENGINE_CORE_API void  MemoryAllocator_Deallocate(void* ptr) {
#ifdef USE_MIMALLOC_ALLOCATOR
		mi_free(ptr);
#else
		ZAllocator_SEMalloc_Virtual::deallocate(ptr);
#endif
	}
	ENGINE_CORE_API void* MemoryAllocator_Calloc(size_t nmemb, size_t size, size_t alignment) {
#ifdef USE_MIMALLOC_ALLOCATOR
		return mi_calloc_aligned(nmemb, size, alignment);
#else
		return ZAllocator_SEMalloc_Virtual::calloc(nmemb, size, alignment);
#endif
	}
	ENGINE_CORE_API void* MemoryAllocator_Realloc(void* ptr, size_t new_size, size_t alignment) {
#ifdef USE_MIMALLOC_ALLOCATOR
		return mi_realloc_aligned(ptr, new_size, alignment);
#else
		return ZAllocator_SEMalloc_Virtual::realloc(ptr, new_size, alignment);
#endif
	}
}