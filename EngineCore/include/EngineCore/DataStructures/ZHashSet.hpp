#include "EngineCore/Hashing/ObjectHasher.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#ifdef STATE_ENGINE_USE_EASTL
#include <EASTL/unordered_set.h>
#include "EastlAllocator.hpp"
#else
#include <unordered_set>
#include "StlAllocator.hpp"
#endif

namespace StateEngine::EngineCore::DataStructures {
#ifndef STATE_ENGINE_USE_EASTL
	template <typename TItem, HashProvider THashProvider = Hashing::Fnv1aHashProvider>
	using ZHashSet = std::unordered_set<TItem, Hashing::ObjectHasher<THashProvider>, std::equal_to<TItem>, StlAllocator<TItem>>;
#else
	template <typename TItem, HashProvider THashProvider = Hashing::Fnv1aHashProvider>
	using ZHashSet = eastl::unordered_set<TItem, Hashing::ObjectHasher<THashProvider>, eastl::equal_to<TItem>, EastlAllocator>;
#endif
}