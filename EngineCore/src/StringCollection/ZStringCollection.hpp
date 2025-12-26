#pragma once
#include <atomic>
#include <mutex>
#include <cassert>

#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/SmartPointers/ZUniquePointer.hpp"
using StateEngine::EngineCore::DataStructures::ZBuffer;
using StateEngine::EngineCore::DataStructures::ZHashMap;
using StateEngine::EngineCore::SmartPointers::ZUniquePointer;

namespace StateEngine::EngineCore::StringCollection {
	class ZStringCollection {
	private:
		struct StringRecordHeader {
			std::atomic<uint32_t> refCount;
			uint32_t stringLength;
			size_t magic;
		};
		static constexpr size_t kStringCollectionHashTablesCount = 8;
		static constexpr size_t kDefaultStringCollectionInitialTableSize = 65536;
		static constexpr size_t kStringRecordHeaderMagic = 0x52545353;

		using StringRecordPointer = ZUniquePointer<StringRecordHeader>;
		using StringRecordsBuffer = ZBuffer<StringRecordPointer>;

		static std::mutex mutexes[kStringCollectionHashTablesCount];
		static ZHashMap<size_t, StringRecordsBuffer> hashToStringMaps[kStringCollectionHashTablesCount];
	
	public:
		inline static void Init() {
			for (size_t i = 0; i < kStringCollectionHashTablesCount; ++i) {
				hashToStringMaps[i].reserve(kDefaultStringCollectionInitialTableSize);
			}
		}
		static const char* GetOrCreateSharedString(const char* str);
		inline static void IncrementRefCount(const char* str) {
			StringRecordHeader* hdr = reinterpret_cast<StringRecordHeader*>(const_cast<char*>(str)) - 1;
			if (hdr->magic != kStringRecordHeaderMagic) {
				assert("ZStringCollection::incrementRefCount - corrupted string header magic");
				return;
			}
			hdr->refCount.fetch_add(1, std::memory_order_relaxed);
		}
		inline static void DecrementRefCount(const char* str) {
			StringRecordHeader* hdr = reinterpret_cast<StringRecordHeader*>(const_cast<char*>(str)) - 1;
			if (hdr->magic != kStringRecordHeaderMagic) {
				assert("ZStringCollection::decrementRefCount - corrupted string header magic");
				return;
			}
			uint32_t oldValue = hdr->refCount.fetch_sub(1, std::memory_order_relaxed);
			if (oldValue == 1)
				RemoveStringRecord(hdr);
		}
		static void RemoveStringRecord(StringRecordHeader* hdr);
	};
}