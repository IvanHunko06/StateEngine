#include "EngineCore/Hashing/ObjectHasher.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#ifdef STATE_ENGINE_USE_EASTL
#include "EASTL/fixed_hash_set.h"
#include "EastlAllocator.hpp"
#else
#error Not implemented ZFixedHashSet
#endif 

namespace StateEngine::EngineCore::DataStructures {

#ifdef STATE_ENGINE_USE_EASTL
	template <typename TItem, size_t MaxCapacity, HashProvider THashProvider = Hashing::Fnv1aHashProvider>
	using ZFixedHashSet = eastl::fixed_hash_set<TItem,
		MaxCapacity,
		MaxCapacity + 1,
		true,
		Hashing::ObjectHasher<THashProvider>, 
		eastl::equal_to<TItem>, 
		false,
		EastlAllocator
	>;
#endif
}