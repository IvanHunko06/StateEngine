#pragma once
#ifdef STATE_ENGINE_USE_EASTL
#include "EASTL/vector.h"
#include "EastlAllocator.hpp"
#else
#include <vector>
#include "StlAllocator.hpp"
#endif 
namespace StateEngine::EngineCore::DataStructures {
#ifndef STATE_ENGINE_USE_EASTL

	template<typename TItem>
	using ZBuffer = std::vector<TItem, StlAllocator<TItem>>;
#else
	template<typename TItem>
	using ZBuffer = eastl::vector<TItem, EastlAllocator>;
#endif
}