#include "ZStringCollection.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"

using namespace StateEngine::EngineCore::StringCollection;
using StateEngine::EngineCore::Hashing::Fnv1aHashProvider;

ZHashMap<size_t, ZStringCollection::StringRecordsBuffer> ZStringCollection::hashToStringMaps[kStringCollectionHashTablesCount];
std::mutex ZStringCollection::mutexes[kStringCollectionHashTablesCount];

const char* ZStringCollection::getOrCreateSharedString(const char* str) {
	if (!str) return nullptr;
	size_t inputStringLength = strlen(str);
	size_t hash = Fnv1aHashProvider::hashBytes(str, inputStringLength);
	uint32_t tableIndex = static_cast<uint32_t>(hash % kStringCollectionHashTablesCount);
	std::lock_guard<std::mutex> lock(mutexes[tableIndex]);

	auto& hashTable = hashToStringMaps[tableIndex];
	if (!hashTable.contains(hash)) {
		StringRecordsBuffer buffer;
		StringRecordHeader* newStringHeader = reinterpret_cast<StringRecordHeader*>(
			MemoryAllocator_AlignedAllocate(sizeof(StringRecordHeader) + inputStringLength + 1,
				alignof(StringRecordHeader))
		);
		if (!newStringHeader) {
			assert(newStringHeader && "ZStringCollection::GetOrCreateSharedString - allocation failed");
			return nullptr;
		}
		newStringHeader->refCount.store(1, std::memory_order_relaxed);
		newStringHeader->stringLength = inputStringLength + 1;
		newStringHeader->magic = kStringRecordHeaderMagic;

		char* stringData = reinterpret_cast<char*>(newStringHeader + 1);
		memcpy(stringData, str, inputStringLength);
		stringData[inputStringLength] = '\0';

		buffer.push_back(StringRecordPointer::makeFromPointer(newStringHeader));
		hashTable[hash] = std::move(buffer);

		return const_cast<const char*>(stringData);
	}
	auto& buffer = hashTable[hash];
	for (auto& curHeader : buffer) {
		if (curHeader->stringLength - 1 != inputStringLength)
			continue;
		const char* curString = reinterpret_cast<const char*>(curHeader.get() + 1);
		if (strcmp(str, curString) == 0) {
			curHeader->refCount.fetch_add(1, std::memory_order_relaxed);
			return curString;
		}
	}
	StringRecordHeader* newStringHeader = reinterpret_cast<StringRecordHeader*>(
		MemoryAllocator_AlignedAllocate(
			sizeof(StringRecordHeader) + inputStringLength + 1,
			alignof(StringRecordHeader))
		);
	if (!newStringHeader) {
		assert(newStringHeader && "ZStringCollection::GetOrCreateSharedString - allocation failed");
		return nullptr;
	}
	newStringHeader->refCount.store(1, std::memory_order_relaxed);
	newStringHeader->stringLength = inputStringLength + 1;
	newStringHeader->magic = kStringRecordHeaderMagic;
	char* stringData = reinterpret_cast<char*>(newStringHeader + 1);
	memcpy(stringData, str, inputStringLength);
	stringData[inputStringLength] = '\0';
	buffer.push_back(StringRecordPointer::makeFromPointer(newStringHeader));

	return const_cast<const char*>(stringData);
}
void ZStringCollection::removeStringRecord(StringRecordHeader* hdr) {
	const char* inputStringData = reinterpret_cast<char*>(hdr + 1);
	size_t inputStringLength = strlen(inputStringData);
	size_t hash = Fnv1aHashProvider::hashBytes(inputStringData, inputStringLength);
	uint32_t tableIndex = static_cast<uint32_t>(hash % kStringCollectionHashTablesCount);
	std::lock_guard<std::mutex> lock(mutexes[tableIndex]);
	auto& hashTable = hashToStringMaps[tableIndex];
	auto& buffer = hashTable[hash];

	size_t index = 0;
	for (const auto& curHdr : buffer) {
		if (curHdr.get() == hdr)
			break;
		++index;
	}

	if (index >= buffer.size())
		return;
	buffer.erase(buffer.begin() + index);
	if (buffer.empty()) {
		hashTable.erase(hash);
	}
}