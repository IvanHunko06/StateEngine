#pragma once
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"

namespace StateEngine::EngineCore::DataStructures {

	template<typename T>
	struct StlAllocator {
		using value_type = T;
		StlAllocator() noexcept = default;
		template<class U>
		StlAllocator(const StlAllocator<U>&) noexcept {}

		T* allocate(size_t n) {
			return reinterpret_cast<T*>(MemoryAllocator_AlignedAllocate(n * sizeof(T), alignof(T)));
		}

		void deallocate(T* p, size_t sz) {
			MemoryAllocator_Deallocate(p);
		}

		bool operator==(const StlAllocator&) const noexcept { return true; }
		bool operator!=(const StlAllocator&) const noexcept { return false; }
	};
}