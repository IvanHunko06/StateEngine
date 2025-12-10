#pragma once
#ifdef STATE_ENGINE_USE_EASTL
#include "EASTL/fixed_vector.h"
#include "EastlAllocator.hpp"
#else
#error Not implemented ZStackBuffer
#endif 
namespace StateEngine::EngineCore::DataStructures {
#if STATE_ENGINE_USE_EASTL
	template<typename TItem, size_t MaxCapacity>
	using ZFixedBuffer = eastl::fixed_vector<TItem, MaxCapacity, true, EastlAllocator>;
#endif
}