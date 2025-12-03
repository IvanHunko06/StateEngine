#pragma once
#include "ConvertableToItemConcept.hpp"
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"
#include "EngineCore/Hashing/HashProviderConcept.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#include "EngineCore/Hashing/ObjectHasher.hpp"
#ifdef STATE_ENGINE_USE_EASTL
#include <EASTL/unordered_map.h>
#include "EastlAllocator.hpp"	
#else
#include <unordered_map>
#include "StlAllocator.hpp"
#endif


namespace StateEngine::EngineCore::DataStructures {
#ifndef STATE_ENGINE_USE_EASTL
	template<typename TKey, typename TValue, HashProvider THashProvider = Hashing::Fnv1aHashProvider>
	using ZHashMap = std::unordered_map<TKey, TValue, Hashing::ObjectHasher<THashProvider>, std::equal_to<TKey>, StlAllocator<std::pair<const TKey, TValue>>>;
#else
	template<typename TKey, typename TValue, HashProvider THashProvider = Hashing::Fnv1aHashProvider>
	using ZHashMap = eastl::unordered_map<TKey, TValue, Hashing::ObjectHasher<THashProvider>, eastl::equal_to<TKey>, EastlAllocator>;
#endif
}