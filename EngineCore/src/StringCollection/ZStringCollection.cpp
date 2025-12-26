#include "ZStringCollection.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <utility>

using namespace StateEngine::EngineCore::StringCollection;
using StateEngine::EngineCore::Hashing::Fnv1aHashProvider;

ZHashMap<size_t, ZStringCollection::StringRecordsBuffer>
    ZStringCollection::hashToStringMaps[kStringCollectionHashTablesCount];
std::mutex ZStringCollection::mutexes[kStringCollectionHashTablesCount];

const char* ZStringCollection::GetOrCreateSharedString(const char* str)
{
    if (str == nullptr) {
        return nullptr;
    }
    const size_t inputStringLength = strlen(str);
    const size_t hash              = Fnv1aHashProvider::HashBytes(str, inputStringLength);
    const uint32_t tableIndex      = static_cast<uint32_t>(hash % kStringCollectionHashTablesCount);
    const std::lock_guard<std::mutex> lock(mutexes[tableIndex]);

    auto& hashTable = hashToStringMaps[tableIndex];
    if (!hashTable.contains(hash)) {
        StringRecordsBuffer buffer;
        auto* newStringHeader = reinterpret_cast<StringRecordHeader*>(MemoryAllocator_AlignedAllocate(
            sizeof(StringRecordHeader) + inputStringLength + 1, alignof(StringRecordHeader)));
        if (newStringHeader == nullptr) {
            assert(newStringHeader && "ZStringCollection::GetOrCreateSharedString - allocation failed");
            return nullptr;
        }
        newStringHeader->refCount.store(1, std::memory_order_relaxed);
        newStringHeader->stringLength = inputStringLength + 1;
        newStringHeader->magic        = kStringRecordHeaderMagic;

        char* stringData = reinterpret_cast<char*>(newStringHeader + 1);
        memcpy(stringData, str, inputStringLength);
        stringData[inputStringLength] = '\0';

        buffer.push_back(StringRecordPointer::makeFromPointer(newStringHeader));
        hashTable[hash] = std::move(buffer);

        return const_cast<const char*>(stringData);
    }
    auto& buffer = hashTable[hash];
    for (auto& curHeader : buffer) {
        if (curHeader->stringLength - 1 != inputStringLength) {
            continue;
        }
        const char* curString = reinterpret_cast<const char*>(curHeader.get() + 1);
        if (strcmp(str, curString) == 0) {
            curHeader->refCount.fetch_add(1, std::memory_order_relaxed);
            return curString;
        }
    }
    auto* newStringHeader = reinterpret_cast<StringRecordHeader*>(MemoryAllocator_AlignedAllocate(
        sizeof(StringRecordHeader) + inputStringLength + 1, alignof(StringRecordHeader)));
    if (newStringHeader == nullptr) {
        assert(newStringHeader && "ZStringCollection::GetOrCreateSharedString - allocation failed");
        return nullptr;
    }
    newStringHeader->refCount.store(1, std::memory_order_relaxed);
    newStringHeader->stringLength = inputStringLength + 1;
    newStringHeader->magic        = kStringRecordHeaderMagic;
    char* stringData              = reinterpret_cast<char*>(newStringHeader + 1);
    memcpy(stringData, str, inputStringLength);
    stringData[inputStringLength] = '\0';
    buffer.push_back(StringRecordPointer::makeFromPointer(newStringHeader));

    return const_cast<const char*>(stringData);
}
void ZStringCollection::RemoveStringRecord(StringRecordHeader* hdr)
{
    const char* inputStringData    = reinterpret_cast<char*>(hdr + 1);
    const size_t inputStringLength = strlen(inputStringData);
    const size_t hash              = Fnv1aHashProvider::HashBytes(inputStringData, inputStringLength);
    const uint32_t tableIndex      = static_cast<uint32_t>(hash % kStringCollectionHashTablesCount);
    const std::lock_guard<std::mutex> lock(mutexes[tableIndex]);
    auto& hashTable = hashToStringMaps[tableIndex];
    auto& buffer    = hashTable[hash];

    size_t index = 0;
    for (const auto& curHdr : buffer) {
        if (curHdr.get() == hdr) {
            break;
        }
        ++index;
    }

    if (index >= buffer.size()) {
        return;
    }

    buffer.erase(buffer.begin() + index);
    if (buffer.empty()) {
        hashTable.erase(hash);
    }
}