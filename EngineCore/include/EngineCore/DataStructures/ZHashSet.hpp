#pragma once
#ifdef USE_STL_STRUCTURES
#include "EngineCore/Hashing/ObjectHasher.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#include <unordered_set>
#include "StlAllocator.hpp"
#else
#include "ZHashMap.hpp"
#include "ConvertableToItemConcept.hpp"
#endif

namespace StateEngine::EngineCore::DataStructures {

#ifndef USE_STL_STRUCTURES
	template <typename TItem, HashProvider THashProvider = Hashing::Fnv1aHashProvider>
	class ZHashSet{
	private:
		ZHashMap<TItem, bool, THashProvider> map_;
		using itemsIterator = decltype(map_)::keysIterator;
	public:
		ZHashSet() = default;
		ZHashSet(const ZHashSet& other) noexcept {
			map_ = other.map_;
		}
		ZHashSet(ZHashSet&& other) noexcept {
			map_ = std::move(other.map_);
		}

		ZHashSet& operator=(const ZHashSet& other) noexcept{
			if (this != &other) {
				map_ = other.map_;
			}
			return *this;
		}
		ZHashSet& operator=(ZHashSet&& other) noexcept {
			if (this != &other) {
				map_ = std::move(other.map_);
			}
			return *this;
		}

		inline bool contains(const TItem& item) const noexcept {
			return map_.containsKey(item);
		}

		template <ConvertibleToItem<TItem> T>
		inline void insert(T&& item) {
			map_.insert(std::forward<T>(item), true);
		}

		inline void remove(const TItem& item) noexcept{
			map_.remove(item);
		}
		inline void resize(uint32_t newSize) noexcept{
			map_.resize(newSize);
		}

	public:
		inline itemsIterator begin() const {
			return map_.keys().begin();
		}
		inline itemsIterator end() const {
			return map_.keys().end();
		}
	};
#else
	template <typename TItem, HashProvider THashProvider = Hashing::Fnv1aHashProvider>
	using ZHashSet = std::unordered_set<TItem, Hashing::ObjectHasher<THashProvider>, std::equal_to<TItem>, StlAllocator<TItem>>;
#endif
}