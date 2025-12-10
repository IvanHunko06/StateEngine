#pragma once
#include "EngineCore/Hashing/HashProviderConcept.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#include "EngineCore/Hashing/ObjectHasher.hpp"
#ifdef STATE_ENGINE_USE_EASTL
#include <EASTL/fixed_hash_map.h>
#include "EastlAllocator.hpp"	
#else
#error Not implemented ZFixedHashMap
#endif 


namespace StateEngine::EngineCore::DataStructures {
#ifdef STATE_ENGINE_USE_EASTL
	template<typename TKey, typename TValue, size_t MaxCapacity, HashProvider THashProvider = Hashing::Fnv1aHashProvider>
	using ZFixedHashMap = eastl::fixed_hash_map<
		TKey, 
		TValue, 
		MaxCapacity, 
		MaxCapacity + 1, 
		true, 
		Hashing::ObjectHasher<THashProvider>, 
		eastl::equal_to<TKey>, 
		false, 
		EastlAllocator
	>;
#endif
}