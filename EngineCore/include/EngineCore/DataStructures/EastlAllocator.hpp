#pragma once
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"
#include <cstddef>
namespace StateEngine::EngineCore::DataStructures {

	struct EastlAllocator {
		using size_type = size_t;
		using difference_type = ptrdiff_t;
		EastlAllocator() noexcept = default;
		EastlAllocator(const char* = "Global Allocator") noexcept {}

		void* allocate(size_t n, int flags = 0) {
			return MemoryAllocator_AlignedAllocate(n, alignof(std::max_align_t));
		}
		void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0) {
			return MemoryAllocator_AlignedAllocate(n, alignment);
		}

		void deallocate(void* p, size_t sz) {
			MemoryAllocator_Deallocate(p);
		}

		bool operator==(const EastlAllocator&) const noexcept { return true; }
		bool operator!=(const EastlAllocator&) const noexcept { return false; }
	};
}