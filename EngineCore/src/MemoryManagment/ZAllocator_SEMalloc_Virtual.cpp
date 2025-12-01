#include "ZAllocator_SEMalloc_Virtual.hpp"
#include <cassert>

using namespace StateEngine::EngineCore::MemoryManagment;
thread_local ZAllocator_SEMalloc_Virtual::ThreadLocalControlBlock ZAllocator_SEMalloc_Virtual::threadLocalControlBlock_;
ZAllocator_SEMalloc_Virtual::MemoryArenaInfo* ZAllocator_SEMalloc_Virtual::allArenasHead_ = nullptr;
std::shared_mutex ZAllocator_SEMalloc_Virtual::allArenasMutex_;

void* ZAllocator_SEMalloc_Virtual::allocate(size_t sz) {
	return alignedAllocate(sz, kDefaultAlignment);
}
void* ZAllocator_SEMalloc_Virtual::alignedAllocate(size_t sz, size_t alignment) {
	if (!sz) return nullptr;
	if (!alignment) alignment = kDefaultAlignment;
	if (alignment > 64) return nullptr;

	if(sz < kLargeAllocThreshold) {
		return allocateFromCachedPool(sz, alignment);
	} else {
		return allocateFromSystem(sz, alignment);
	}
}
void ZAllocator_SEMalloc_Virtual::deallocate(void* ptr) {
	if(!ptr || !isValidAddress(ptr))
		return; // Null or invalid pointer

	MemoryArenaInfo* arena = reinterpret_cast<MemoryArenaInfo*>(ptr) - 1;
	MemoryChunkHeader* chunkHeader = reinterpret_cast<MemoryChunkHeader*>(ptr) - 1;
	uint8_t padding = reinterpret_cast<uint8_t*>(ptr)[-1];
	if (arena->magic != kArenaHeaderMagic && chunkHeader->magic != kChunkHeaderMagic) {
		arena = reinterpret_cast<MemoryArenaInfo*>(
			reinterpret_cast<uintptr_t>(ptr) - padding - sizeof(MemoryArenaInfo));
		chunkHeader = reinterpret_cast<MemoryChunkHeader*>(
			reinterpret_cast<uintptr_t>(ptr) - padding - sizeof(MemoryChunkHeader));
	}
	if (arena->magic == kArenaHeaderMagic) {
		threadLocalControlBlock_.removeMemoryArena(arena);
		PageAllocator::tryRelease(arena->basePtr);
		return;
	}
	if(chunkHeader->magic != kChunkHeaderMagic)
		return; // Invalid chunk header

	uint32_t chunkSpanClass = getChunkSpanClassFromSize(chunkHeader->payloadSize);
	std::thread::id curThread = std::this_thread::get_id();
	std::thread::id ownerThread;
	MemoryArenaInfo* ownedArena = nullptr;
	switch (chunkSpanClass)
	{
	case 0: {
		auto* span64 = ChunkSpan64::getSpanFromChunk(chunkHeader);
		if (!span64->isUsed(chunkHeader->spanItemIndex)) {
			__debugbreak();
			assert(false && "attempt to deallocate already deallocated memory");
			return;
		}
		ownedArena = span64->getOwnedArena();
		ownerThread = span64->getOwnedThread();
		span64->markFree(chunkHeader->spanItemIndex);
		break;
	}
	case 1: {
		auto* span32 = ChunkSpan32::getSpanFromChunk(chunkHeader);
		if (!span32->isUsed(chunkHeader->spanItemIndex)) {
			__debugbreak();
			assert(false && "attempt to deallocate already deallocated memory");
			return;
		}
		ownedArena = span32->getOwnedArena();
		ownerThread = span32->getOwnedThread();
		span32->markFree(chunkHeader->spanItemIndex);
		break;
	}
	case 2: {
		auto* span16 = ChunkSpan16::getSpanFromChunk(chunkHeader);
		if (!span16->isUsed(chunkHeader->spanItemIndex)) {
			__debugbreak();
			assert(false && "attempt to deallocate already deallocated memory");
			return;
		}
		ownedArena = span16->getOwnedArena();
		ownerThread = span16->getOwnedThread();
		span16->markFree(chunkHeader->spanItemIndex);
		break;
	}
	case 3: {
		auto* span1 = ChunkSpan1::getSpanFromChunk(chunkHeader);
		if (!span1->isUsed(chunkHeader->spanItemIndex)) {
			__debugbreak();
			assert(false && "attempt to deallocate already deallocated memory");
			return;
		}
		ownedArena = span1->getOwnedArena();
		ownerThread = span1->getOwnedThread();
		span1->markFree(chunkHeader->spanItemIndex);
		break;
	}
	default: {
		return; // Invalid chunk span class
	}
	}

	if (ownerThread == curThread)
		threadLocalControlBlock_.pushFreeChunk(chunkHeader);
	else
		pushRemoteFree(ownedArena, chunkHeader);

	ownedArena->livedChunkCount.fetch_sub(1, std::memory_order_relaxed);
	bool arenaEmpty = (ownedArena->livedChunkCount.load(std::memory_order_relaxed) == 0);
	bool orphaned = ownedArena->orphaned.load(std::memory_order_acquire);
	if(arenaEmpty && orphaned) {
		threadLocalControlBlock_.removeMemoryArena(ownedArena);
		PageAllocator::tryRelease(ownedArena->basePtr);
	}
}
void* ZAllocator_SEMalloc_Virtual::calloc(size_t nmemb, size_t size) {
	return calloc(nmemb, size, kDefaultAlignment);
}
void* ZAllocator_SEMalloc_Virtual::calloc(size_t nmemb, size_t size, size_t alignment) {
	if (!nmemb || !size) return nullptr;
	size_t toAllocate = nmemb * size;
	void* allocatedMemory = alignedAllocate(toAllocate, alignment);
	if (!allocatedMemory) return nullptr;
	memset(allocatedMemory, 0, toAllocate);
	return allocatedMemory;
}
void* ZAllocator_SEMalloc_Virtual::realloc(void* ptr, size_t new_size) {
	return realloc(ptr, new_size, kDefaultAlignment);
}
void* ZAllocator_SEMalloc_Virtual::realloc(void* ptr, size_t new_size, size_t alignment) {
	if (!ptr)
		return alignedAllocate(new_size, alignment);

	if (!new_size) {
		deallocate(ptr);
		return nullptr;
	}
	MemoryArenaInfo* arena = reinterpret_cast<MemoryArenaInfo*>(ptr) - 1;
	MemoryChunkHeader* chunkHeader = reinterpret_cast<MemoryChunkHeader*>(ptr) - 1;
	uint8_t padding = reinterpret_cast<uint8_t*>(ptr)[-1];
	if (arena->magic != kArenaHeaderMagic && chunkHeader->magic != kChunkHeaderMagic) {
		arena = reinterpret_cast<MemoryArenaInfo*>(
			reinterpret_cast<uintptr_t>(ptr) - padding - sizeof(MemoryArenaInfo));
		chunkHeader = reinterpret_cast<MemoryChunkHeader*>(
			reinterpret_cast<uintptr_t>(ptr) - padding - sizeof(MemoryChunkHeader));
	}
	size_t oldSize = 0;
	if (chunkHeader->magic == kChunkHeaderMagic)
		oldSize = chunkHeader->payloadSize;
	else if (arena->magic == kArenaHeaderMagic)
		oldSize = arena->payloadSize;
	else
		return nullptr;

	if (oldSize >= new_size)
		return ptr;

	void* newPtr = alignedAllocate(new_size, alignment);
	memmove(newPtr, ptr, oldSize);
	deallocate(ptr);
	return newPtr;
}

void* ZAllocator_SEMalloc_Virtual::allocateFromCachedPool(size_t sz, size_t alignment) {
	int32_t szCls = InternalSizeAlignmentClassHelpers::getClassFromSize(sz);
	int32_t alignCls = InternalSizeAlignmentClassHelpers::getClassFromAlignment(alignment);
	
	if(szCls < 0 || alignCls < 0)
		// Invalid size or alignment class
		return nullptr;

	// Try to pop from the free list
	if(++threadLocalControlBlock_.drainAllocationAmountCount_ >= ThreadLocalControlBlock::kDrainAfterAllocationAmount) {
		MemoryArenaInfo* curArena = threadLocalControlBlock_.getMemoryArenasHead();
		do {
			MemoryChunkHeader* curChunk = drainRemoteFrees(curArena);
			while(curChunk) {
				MemoryChunkHeader* nextChunk = curChunk->nextRemoteFree;
				threadLocalControlBlock_.pushFreeChunk(curChunk);
				curChunk = nextChunk;
			}
		} while ((curArena = curArena->nextThreadLocalArena) != nullptr);
		threadLocalControlBlock_.drainAllocationAmountCount_ = 0;
	}

	MemoryChunkHeader* freeChunkHeader = threadLocalControlBlock_.popFreeChunk(szCls, alignCls);
	
	if (freeChunkHeader) {
		uint32_t chunkSpanClass = getChunkSpanClassFromSize(freeChunkHeader->payloadSize);
		MemoryArenaInfo* ownedArena = nullptr;
		switch (chunkSpanClass)
		{
		case 0: {
			auto* span64 = ChunkSpan64::getSpanFromChunk(freeChunkHeader);
			span64->markUsed(freeChunkHeader->spanItemIndex);
			ownedArena = span64->getOwnedArena();
			break;
		}
		case 1: {
			auto* span32 = ChunkSpan32::getSpanFromChunk(freeChunkHeader);
			span32->markUsed(freeChunkHeader->spanItemIndex);
			ownedArena = span32->getOwnedArena();
			break;
		}
		case 2: {
			auto* span16 = ChunkSpan16::getSpanFromChunk(freeChunkHeader);
			span16->markUsed(freeChunkHeader->spanItemIndex);
			ownedArena = span16->getOwnedArena();
			break;
		}
		case 3: {
			auto* span1 = ChunkSpan1::getSpanFromChunk(freeChunkHeader);
			span1->markUsed(freeChunkHeader->spanItemIndex);
			ownedArena = span1->getOwnedArena();
			break;
		}
		default: {
			return nullptr;
		}
		}
		if(!ownedArena)
			return nullptr; // Invalid chunk span
		ownedArena->livedChunkCount.fetch_add(1, std::memory_order_relaxed);
		uintptr_t rawDataPtr = reinterpret_cast<uintptr_t>(freeChunkHeader) + sizeof(MemoryChunkHeader);
		uintptr_t alignedDataPtr = align_up(rawDataPtr, alignment);
		uint8_t padding = static_cast<uint8_t>(alignedDataPtr - rawDataPtr);
		if (padding) {
			reinterpret_cast<uint8_t*>(alignedDataPtr)[-1] = padding;
		}
		return reinterpret_cast<void*>(alignedDataPtr);
	}

	// No free chunk available - try to allocate a new span
	size_t payloadSize = InternalSizeAlignmentClassHelpers::getSizeFromClass(szCls);
	size_t payloadAlignment = InternalSizeAlignmentClassHelpers::getAlignmentFromClass(alignCls);
	AnyChunkSpan newSpan = ñraveSpanAnyArena(payloadSize, payloadAlignment);
	if (std::holds_alternative<std::nullptr_t>(newSpan)) {
		// Failed to allocate a new span
		return nullptr;
	}
	std::visit([&](auto&& span) {
		using T = std::decay_t<decltype(span)>;
		if constexpr (!std::is_same_v<T, std::nullptr_t>) {
			size_t count = span->getChunksCount();
			for (size_t i = 0; i < count; ++i) {
				MemoryChunkHeader* chunkHeader = span->getAt(count - i - 1);
				if (!chunkHeader) continue;
				threadLocalControlBlock_.pushFreeChunk(chunkHeader);
			}
		}
	}, newSpan);

	// Now try to pop again
	freeChunkHeader = threadLocalControlBlock_.popFreeChunk(szCls, alignCls);
	if(!freeChunkHeader)
		return nullptr; // Failed to pop after adding new span
	
	MemoryArenaInfo* ownedArena = nullptr;
	uint32_t chunkSpanClass = getChunkSpanClassFromSize(freeChunkHeader->payloadSize);
	switch (chunkSpanClass)
	{
	case 0: {
		auto* span64 = ChunkSpan64::getSpanFromChunk(freeChunkHeader);
		span64->markUsed(freeChunkHeader->spanItemIndex);
		ownedArena = span64->getOwnedArena();
		break;
	}
	case 1: {
		auto* span32 = ChunkSpan32::getSpanFromChunk(freeChunkHeader);
		span32->markUsed(freeChunkHeader->spanItemIndex);
		ownedArena = span32->getOwnedArena();
		break;
	}
	case 2: {
		auto* span16 = ChunkSpan16::getSpanFromChunk(freeChunkHeader);
		span16->markUsed(freeChunkHeader->spanItemIndex);
		ownedArena = span16->getOwnedArena();
		break;
	}
	case 3: {
		auto* span1 = ChunkSpan1::getSpanFromChunk(freeChunkHeader);
		span1->markUsed(freeChunkHeader->spanItemIndex);
		ownedArena = span1->getOwnedArena();
		break;
	}
	default: {
		return nullptr;
	}
	}
	if(!ownedArena)
		return nullptr; // Invalid chunk span
	ownedArena->livedChunkCount.fetch_add(1, std::memory_order_relaxed);
	uintptr_t rawDataPtr = reinterpret_cast<uintptr_t>(freeChunkHeader) + sizeof(MemoryChunkHeader);
	uintptr_t alignedDataPtr = align_up(rawDataPtr, alignment);
	uint8_t padding = static_cast<uint8_t>(alignedDataPtr - rawDataPtr);
	if (padding) {
		reinterpret_cast<uint8_t*>(alignedDataPtr)[-1] = padding;
	}
	return reinterpret_cast<void*>(alignedDataPtr);
}
void* ZAllocator_SEMalloc_Virtual::allocateFromSystem(size_t sz, size_t alignment) {
	size_t pageSize = PageAllocator::getPageSize();
	size_t toAllocate = align_up(sz + sizeof(MemoryArenaInfo) + alignment, pageSize);
	void* reservedMem = PageAllocator::reserve(toAllocate);
	if (!reservedMem) return nullptr;
	bool commited = PageAllocator::tryCommit(reservedMem, toAllocate);
	if (!commited) {
		PageAllocator::tryRelease(reservedMem);
		return nullptr;
	}
	uintptr_t rawDataPtr = reinterpret_cast<uintptr_t>(reservedMem) + sizeof(MemoryArenaInfo);
	uintptr_t alignedDataPtr = align_up(rawDataPtr, alignment);
	uint8_t padding = static_cast<uint8_t>(alignedDataPtr - rawDataPtr);

	MemoryArenaInfo* newArena = reinterpret_cast<MemoryArenaInfo*>(reservedMem);
	newArena->basePtr = reservedMem;
	newArena->reservedSize = toAllocate;
	newArena->committedSize = toAllocate;
	newArena->freeDataPtr = nullptr; // No free data in this arena
	newArena->isLargeAllocArena = true;
	newArena->livedChunkCount.store(1, std::memory_order_relaxed);
	newArena->orphaned.store(false, std::memory_order_release);
	newArena->remoteFreeListHead.store(nullptr, std::memory_order_relaxed);
	newArena->payloadSize = sz;
	newArena->nextThreadLocalArena = nullptr;
	newArena->nextGlobalArena = nullptr;
	newArena->magic = kArenaHeaderMagic;
	
	threadLocalControlBlock_.pushMemoryArena(newArena);
	if(padding) {
		reinterpret_cast<uint8_t*>(alignedDataPtr)[-1] = padding;
	}
	return reinterpret_cast<void*>(alignedDataPtr);
}

bool ZAllocator_SEMalloc_Virtual::allocateNewArena() {
	size_t pageSize = PageAllocator::getPageSize();
	size_t reserveSize = align_up(threadLocalControlBlock_.memoryReserveSize_, pageSize);
	void* reservedMem = PageAllocator::reserve(reserveSize);
	if (!reservedMem) return false;

	size_t commitSize = align_up(threadLocalControlBlock_.memoryCommitSize_, pageSize);
	bool commited = PageAllocator::tryCommit(reservedMem, commitSize);
	if (!commited) {
		PageAllocator::tryRelease(reservedMem);
		return false;
	}

	threadLocalControlBlock_.memoryReserveSize_ = std::min(threadLocalControlBlock_.memoryReserveSize_ * 2, threadLocalControlBlock_.kMaxReserveSizePerThread);
	threadLocalControlBlock_.memoryCommitSize_ = std::min(threadLocalControlBlock_.memoryCommitSize_ * 2, threadLocalControlBlock_.kMaxCommitSizePerThread);

	MemoryArenaInfo* newArena = reinterpret_cast<MemoryArenaInfo*>(reservedMem);
	newArena->basePtr = reservedMem;
	newArena->reservedSize = reserveSize;
	newArena->committedSize = commitSize;
	newArena->freeDataPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(reservedMem) + sizeof(MemoryArenaInfo));
	newArena->isLargeAllocArena = false;
	newArena->livedChunkCount.store(0, std::memory_order_relaxed);
	newArena->orphaned.store(false, std::memory_order_relaxed);
	newArena->remoteFreeListHead.store(nullptr, std::memory_order_relaxed);
	newArena->nextThreadLocalArena = nullptr;
	newArena->nextGlobalArena = nullptr;
	newArena->magic = kArenaHeaderMagic;
	threadLocalControlBlock_.pushMemoryArena(newArena);
	return true;
}
size_t ZAllocator_SEMalloc_Virtual::tryExpadArena(MemoryArenaInfo* arena, size_t needSize) {
	if (arena->committedSize >= arena->reservedSize) return 0;
	size_t pageSize = PageAllocator::getPageSize();
	size_t available = arena->reservedSize - arena->committedSize;
	size_t toCommit = std::min(align_up(needSize, pageSize), available);
	if (toCommit == 0) return 0;
	void* commitAddr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(arena->basePtr) + arena->committedSize);
	bool commited = PageAllocator::tryCommit(commitAddr, toCommit);
	if (!commited) return 0;
	arena->committedSize += toCommit;
	return toCommit;
}
ZAllocator_SEMalloc_Virtual::AnyChunkSpan ZAllocator_SEMalloc_Virtual::tryCraveSpanInArena(MemoryArenaInfo* arena, size_t chunkSize, size_t alignment) {
	uint32_t spanClass = getChunkSpanClassFromSize(chunkSize);
	size_t bytesNeeded = 0;
	size_t spanAlign = 0;
	switch (spanClass)
	{
	case 0: {
		bytesNeeded = ChunkSpan64::totalBytes(chunkSize, alignment);
		spanAlign = alignof(ChunkSpan64);
		break;
	}
	case 1: {
		bytesNeeded = ChunkSpan32::totalBytes(chunkSize, alignment);
		spanAlign = alignof(ChunkSpan32);
		break;
	}
	case 2: {
		bytesNeeded = ChunkSpan16::totalBytes(chunkSize, alignment);
		spanAlign = alignof(ChunkSpan16);
		break;
	}
	case 3: {
		bytesNeeded = ChunkSpan1::totalBytes(chunkSize, alignment);
		spanAlign = alignof(ChunkSpan1);
		break;
	}
	default: {
		return nullptr;
	}
	}

	uintptr_t currentFreePtr = reinterpret_cast<uintptr_t>(arena->freeDataPtr);
	uintptr_t arenaStart = reinterpret_cast<uintptr_t>(arena->basePtr);
	uintptr_t arenaEnd = arenaStart + arena->committedSize;

	uintptr_t alignedPos = align_up(currentFreePtr, spanAlign);
	if (alignedPos + bytesNeeded <= arenaEnd) {
		// We have enough space in the arena. Allocate the span here.
		void* place = reinterpret_cast<void*>(alignedPos);
		arena->freeDataPtr = reinterpret_cast<void*>(alignedPos + bytesNeeded);
		switch (spanClass)
		{
		case 0:
			return new(place) ChunkSpan64(chunkSize, alignment, arena, std::this_thread::get_id());
		case 1:
			return new(place) ChunkSpan32(chunkSize, alignment, arena, std::this_thread::get_id());
		case 2:
			return new(place) ChunkSpan16(chunkSize, alignment, arena, std::this_thread::get_id());
		case 3:
			return new(place) ChunkSpan1(chunkSize, alignment, arena, std::this_thread::get_id());
		default:
			return nullptr;
		}
	}
	// Not enough committed memory - try to commit more for this arena
	size_t need = (alignedPos + bytesNeeded) - arenaEnd;
	size_t committed = tryExpadArena(arena, need);
	if (committed == 0) return nullptr;

	// Recompute end and try again
	arenaEnd = arenaStart + arena->committedSize;

	if (alignedPos + bytesNeeded <= arenaEnd) {
		void* place = reinterpret_cast<void*>(alignedPos);
		arena->freeDataPtr = reinterpret_cast<void*>(alignedPos + bytesNeeded);
		switch (spanClass)
		{
		case 0:
			return new(place) ChunkSpan64(chunkSize, alignment, arena, std::this_thread::get_id());
		case 1:
			return new(place) ChunkSpan32(chunkSize, alignment, arena, std::this_thread::get_id());
		case 2:
			return new(place) ChunkSpan16(chunkSize, alignment, arena, std::this_thread::get_id());
		case 3:
			return new(place) ChunkSpan1(chunkSize, alignment, arena, std::this_thread::get_id());
		default:
			return nullptr;
		}
	}
	return nullptr;
}
ZAllocator_SEMalloc_Virtual::AnyChunkSpan ZAllocator_SEMalloc_Virtual::ñraveSpanAnyArena(size_t chunkSize, size_t alignment) {
	// First, try existing thread-local arenas
	if (threadLocalControlBlock_.getMemoryArenasHead() != nullptr) {
		MemoryArenaInfo* curArena = threadLocalControlBlock_.getMemoryArenasHead();
		do {
			if(curArena->isLargeAllocArena)
				continue; // Skip large alloc arenas
			AnyChunkSpan res = tryCraveSpanInArena(curArena, chunkSize, alignment);
			if (!std::holds_alternative<std::nullptr_t>(res)) {
				return res;
			}
		} while (curArena = curArena->nextThreadLocalArena);
	}
	// Try to allocate a new arena
	if (allocateNewArena()) {
		MemoryArenaInfo* newArena = threadLocalControlBlock_.getMemoryArenasHead();
		if (newArena) {
			AnyChunkSpan res = tryCraveSpanInArena(newArena, chunkSize, alignment);
			if (!std::holds_alternative<std::nullptr_t>(res)) {
				return res;
			}
		}
	}
	return nullptr;
}

void ZAllocator_SEMalloc_Virtual::releaseAllArenas() {
	std::unique_lock<std::shared_mutex> lock(allArenasMutex_);
	do {
		MemoryArenaInfo* curArena = allArenasHead_;
		if (!curArena) return;
		removeArenaFromGlobalList(curArena);
		PageAllocator::tryRelease(curArena->basePtr);
	} while (true);
}