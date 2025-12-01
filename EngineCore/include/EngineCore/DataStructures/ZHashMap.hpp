#pragma once
#include <cassert>
#include <optional>

#include "ConvertableToItemConcept.hpp"
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"
#include "EngineCore/Hashing/HashProviderConcept.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#include "EngineCore/Hashing/ObjectHasher.hpp"
#ifdef USE_STL_STRUCTURES
#include <unordered_map>
#include "StlAllocator.hpp"
#endif


namespace StateEngine::EngineCore::DataStructures {

#ifndef USE_STL_STRUCTURES
	template<typename TKey, typename TValue, HashProvider THashProvider = Hashing::Fnv1aHashProvider>
	class ZHashMap{
	private:
		struct ZHashMapRecord {
			int32_t nextRecordIndex{ -1 };
			TKey key;
			TValue value;
		};
		using ProvidedObjectHasher = Hashing::ObjectHasher<THashProvider>;
	private:
		uint32_t tableSize{ 0 };
		int32_t freeRecordsHeader{ -1 };
		uint32_t recordsCount{ 0 };
		mutable ProvidedObjectHasher hasher{};
		ZHashMapRecord* recordsPtr{ nullptr };
		int32_t* chainsPtr{ nullptr };

	private:
		void reset() {
			tableSize = 0;
			recordsCount = 0;
			freeRecordsHeader = -1;
			recordsPtr = nullptr;
			chainsPtr = nullptr;
		}
	public:
		ZHashMap(const ZHashMap& other) {
			resize(other.tableSize);
			for (const ZHashMapRecord& record : other) {
				insert(record.key, record.value);
			}
		}
		ZHashMap(ZHashMap&& other) noexcept
			:tableSize(other.tableSize),
			freeRecordsHeader(other.freeRecordsHeader),
			recordsCount(other.recordsCount),
			recordsPtr(other.recordsPtr),
			chainsPtr(other.chainsPtr)
		{
			other.reset();
		}
		ZHashMap() = default;
		explicit ZHashMap(uint32_t initialSize) noexcept{
			resize(initialSize);
		}
		~ZHashMap() {
			if (!recordsPtr) return;
			clear();
			MemoryAllocator_Deallocate(recordsPtr);
			reset();
		}

		ZHashMap& operator=(ZHashMap&& other) noexcept{
			if (this != &other) {
				clear();
				if (recordsPtr)
					MemoryAllocator_Deallocate(recordsPtr);

				tableSize = other.tableSize;
				freeRecordsHeader = other.freeRecordsHeader;
				recordsCount = other.recordsCount;
				recordsPtr = other.recordsPtr;
				chainsPtr = other.chainsPtr;
				other.reset();
			}
			return *this;
		}
		ZHashMap& operator=(const ZHashMap& other) {
			if (this != &other) {
				clear();
				if (recordsPtr)
					MemoryAllocator_Deallocate(recordsPtr);

				resize(other.tableSize);
				for (const ZHashMapRecord& record : other) {
					insert(record.key, record.value);
				}
			}
			return *this;
		}

		void resize(uint32_t newSize) noexcept{

			newSize = (std::max)(newSize, 4u);
			newSize = (std::max)(recordsCount, newSize);

			ZHashMapRecord* oldRecordsPtr = recordsPtr;
			int32_t* oldChainsPtr = chainsPtr;
			uint32_t oldTableSize = tableSize;
			uint32_t oldRecordsCount = recordsCount;

			const size_t sizeToAllocate = sizeof(ZHashMapRecord) * newSize + sizeof(int32_t) * newSize;
			recordsPtr = reinterpret_cast<ZHashMapRecord*>(MemoryAllocator_AlignedAllocate(sizeToAllocate, alignof(ZHashMapRecord)));
			if (!recordsPtr) {
				recordsPtr = oldRecordsPtr;
				chainsPtr = oldChainsPtr;
				assert(recordsPtr && "ZHashMap::resize - failed to allocate memory for new records");
				return;
			}

			chainsPtr = reinterpret_cast<int32_t*>(recordsPtr + newSize);
			tableSize = newSize;
			recordsCount = 0;
			freeRecordsHeader = -1;

			std::fill(chainsPtr, chainsPtr + newSize, -1);
			memset(recordsPtr, 0, sizeof(ZHashMapRecord) * newSize);
			//std::fill(recordsPtr, recordsPtr + newSize, 0);

			if (oldRecordsPtr) {
				for (uint32_t bucketIndex = 0; bucketIndex < oldTableSize; ++bucketIndex) {
					int32_t recordIndex = oldChainsPtr[bucketIndex];

					while (recordIndex != -1) {
						ZHashMapRecord* oldRecord = &oldRecordsPtr[recordIndex];
						size_t hash = hasher(oldRecord->key);
						uint32_t bucketIndex = hash % tableSize;
						recordsPtr[recordsCount] = ZHashMapRecord{ chainsPtr[bucketIndex], std::move(oldRecord->key), std::move(oldRecord->value) };
						chainsPtr[bucketIndex] = recordsCount;
						recordsCount++;
						recordIndex = oldRecord->nextRecordIndex;
					}
				}

				MemoryAllocator_Deallocate(oldRecordsPtr);
			}
		}

		inline std::optional<std::reference_wrapper<TValue>> getValue(const TKey& key) const noexcept {
			if (!recordsPtr || !chainsPtr || tableSize == 0) {
				return std::nullopt;
			}

			size_t hash = hasher(key);
			uint32_t bucketIndex = hash % tableSize;

			int32_t currentRecordIndex = chainsPtr[bucketIndex];
			while (currentRecordIndex != -1) {
				ZHashMapRecord* currentRecord = &recordsPtr[currentRecordIndex];

				if (currentRecord->key == key) {
					return currentRecord->value;
				}

				currentRecordIndex = currentRecord->nextRecordIndex;
			}

			return std::nullopt;
		}
		
		template<ConvertibleToItem<TKey> K, ConvertibleToItem<TValue> V>
		inline void insert(K&& key, V&& value) noexcept{
			if (containsKey(key)) return;
			int32_t newRecordIndex;
			if (freeRecordsHeader != -1) {
				newRecordIndex = freeRecordsHeader;
				freeRecordsHeader = recordsPtr[newRecordIndex].nextRecordIndex;
				++recordsCount;
			}
			else {
				if (recordsCount >= tableSize) {
					resize(tableSize * 2);
				}
				newRecordIndex = recordsCount++;
			}

			size_t hash = hasher(key);
			uint32_t bucketIndex = hash % tableSize;

			new (&recordsPtr[newRecordIndex].key) TKey(std::forward<K>(key));
			new (&recordsPtr[newRecordIndex].value) TValue(std::forward<V>(value));
			recordsPtr[newRecordIndex].nextRecordIndex = chainsPtr[bucketIndex];
			chainsPtr[bucketIndex] = newRecordIndex;
		}

		inline bool containsKey(const TKey& key) const noexcept{
			return getValue(key).has_value();
		}

		inline void remove(const TKey& key) noexcept {
			if (!containsKey(key)) return;

			size_t hash = hasher(key);
			uint32_t bucketIndex = hash % tableSize;

			int32_t prevIndex = -1;
			int32_t currentIndex = chainsPtr[bucketIndex];

			while (currentIndex != -1) {
				ZHashMapRecord* currentRecord = &recordsPtr[currentIndex];
				int32_t nextIndex = currentRecord->nextRecordIndex;

				if (currentRecord->key == key) {
					if (prevIndex == -1) {
						chainsPtr[bucketIndex] = nextIndex;
					}
					else {
						recordsPtr[prevIndex].nextRecordIndex = nextIndex;
					}

					currentRecord->nextRecordIndex = freeRecordsHeader;
					freeRecordsHeader = currentIndex;
					currentRecord->value.~TValue();
					currentRecord->key.~TKey();
					recordsCount--;

					return;
				}

				prevIndex = currentIndex;
				currentIndex = nextIndex;
			}
		}

		inline void clear() noexcept{
			if (!recordsCount)return;
			for (uint32_t bucketIndex = 0; bucketIndex < tableSize; ++bucketIndex) {
				int32_t recordIndex = chainsPtr[bucketIndex];
				while (recordIndex != -1) {
					ZHashMapRecord* record = &recordsPtr[recordIndex];
					record->key.~TKey();
					record->value.~TValue();
					recordIndex = record->nextRecordIndex;
				}
			}
			recordsCount = 0;
		}

	public:
		struct keysIterator {
		private:
			ZHashMapRecord* recordsPtr;
			int32_t* chainsPtr;
			uint32_t tableSize;
			uint32_t bucketIndex;
			int32_t recordIndex;
		public:
			keysIterator(ZHashMapRecord* r, int32_t* c, uint32_t size, uint32_t bucket, int32_t rec)
				: recordsPtr(r), chainsPtr(c), tableSize(size),
				bucketIndex(bucket), recordIndex(rec) {
			}
			const TKey& operator*() const {
				return recordsPtr[recordIndex].key;
			}
			const TKey* operator->() const {
				return &(recordsPtr[recordIndex].key);
			}
			keysIterator& operator++() {
				if (recordIndex != -1) {
					recordIndex = recordsPtr[recordIndex].nextRecordIndex;
					if (recordIndex != -1) return *this;
				}
				while (++bucketIndex < tableSize) {
					recordIndex = chainsPtr[bucketIndex];
					if (recordIndex != -1) break;
				}
				return *this;
			}
			keysIterator operator++(int) {
				keysIterator temp = *this;
				++(*this);
				return temp;
			}
			bool operator==(const keysIterator& other) const {
				return recordIndex == other.recordIndex && bucketIndex == other.bucketIndex;
			}

			bool operator!=(const keysIterator& other) const {
				return !(*this == other);
			}
		};
		struct KeyRange {
		private:
			ZHashMapRecord* recordsPtr;
			int32_t* chainsPtr;
			uint32_t tableSize;
		public:
			KeyRange(ZHashMapRecord* r, int32_t* c, uint32_t size)
				: recordsPtr(r), chainsPtr(c), tableSize(size) {
			}

			auto begin() const {
				for (uint32_t bucket = 0; bucket < tableSize; ++bucket) {
					if (chainsPtr[bucket] != -1)
						return keysIterator(recordsPtr, chainsPtr, tableSize, bucket, chainsPtr[bucket]);
				}
				return end();
			}

			auto end() const {
				return keysIterator(recordsPtr, chainsPtr, tableSize, tableSize, -1);
			}
		};
		KeyRange keys() const {
			return KeyRange(recordsPtr, chainsPtr, tableSize);
		}

	public:
		template <bool IsConst>
		struct iterator_impl {
		private:
			using RecordPtr = std::conditional_t<IsConst, const ZHashMapRecord*, ZHashMapRecord*>;
			using ChainsPtr = std::conditional_t<IsConst, const int32_t*, int32_t*>;
			using RecordValue = std::conditional_t<IsConst, const TValue, TValue>;
			using PairRef = std::pair<const TKey&, RecordValue&>;

			RecordPtr recordsPtr;
			ChainsPtr chainsPtr;
			uint32_t tableSize;
			uint32_t bucketIndex;
			int32_t recordIndex;
		public:
			iterator_impl(RecordPtr r, ChainsPtr c, uint32_t size, uint32_t bucket, int32_t rec)
				: recordsPtr(r), chainsPtr(c), tableSize(size),
				bucketIndex(bucket), recordIndex(rec) {
			}

			PairRef operator*() const {
				return PairRef(recordsPtr[recordIndex].key, recordsPtr[recordIndex].value);
			}

			struct ArrowProxy {
				PairRef pair_;
				PairRef* operator->() { return &pair_; }
			};

			ArrowProxy operator->() const {
				return ArrowProxy{ operator*() };
			}

			iterator_impl& operator++() {
				if (recordIndex != -1) {
					recordIndex = recordsPtr[recordIndex].nextRecordIndex;
					if (recordIndex != -1) return *this;
				}
				while (++bucketIndex < tableSize) {
					recordIndex = chainsPtr[bucketIndex];
					if (recordIndex != -1) break;
				}
				return *this;
			}

			bool operator==(const iterator_impl& other) const {
				return recordIndex == other.recordIndex && bucketIndex == other.bucketIndex;
			}
			bool operator!=(const iterator_impl& other) const { return !(*this == other); }
		};

		using standartIterator = iterator_impl<false>;
		using constIterator = iterator_impl<true>;

		standartIterator begin() {
			for (uint32_t bucket = 0; bucket < tableSize; ++bucket) {
				if (chainsPtr[bucket] != -1)
					return standartIterator(recordsPtr, chainsPtr, tableSize, bucket, chainsPtr[bucket]);
			}
			return end();
		}
		standartIterator end() {
			return standartIterator(recordsPtr, chainsPtr, tableSize, tableSize, -1);
		}
		constIterator begin() const {
			for (uint32_t bucket = 0; bucket < tableSize; ++bucket) {
				if (chainsPtr[bucket] != -1)
					return constIterator(recordsPtr, chainsPtr, tableSize, bucket, chainsPtr[bucket]);
			}
			return end();
		}
		constIterator end() const {
			return constIterator(recordsPtr, chainsPtr, tableSize, tableSize, -1);
		}
	};
#else
template<typename TKey, typename TValue, HashProvider THashProvider = Hashing::Fnv1aHashProvider>
using ZHashMap = std::unordered_map<TKey, TValue, Hashing::ObjectHasher<THashProvider>, std::equal_to<TKey>, StlAllocator<std::pair<const TKey, TValue>>>;
#endif
}