#pragma once
#include <cstdint>
#include <atomic>
#include <thread>
#include <variant>
#include <shared_mutex>
#include "AllocatorSizeAlignmentClassHelpers.hpp"
#include "EngineCore/Threading/ZAtomicBitset.hpp"
#include "PageAllocator.hpp"

using StateEngine::EngineCore::Threading::ZAtomicBitset;
namespace StateEngine::EngineCore::MemoryManagment {
	class ZAllocator_SEMalloc_Virtual {
	private:
		// ---------- Tunables ---------------------------------------------------------------
		static constexpr size_t kDefaultAlignment = 16;
		static constexpr size_t kLargeAllocThreshold = 1024 * 1024; // 1 MB
		static constexpr size_t kPowerOfTwoUpperBound = 256;
		static constexpr size_t kPowerOfTwoLowerBound = 8;
		static constexpr size_t kSupportedAlignmentsCount = 3; // 16, 32, 64
		static constexpr size_t kChunkHeaderMagic = 0x4B48434D;
		static constexpr size_t kArenaHeaderMagic = 0x414E5241;
	public:
		// ---------- Allocator --------------------------------------------------------------
		ZAllocator_SEMalloc_Virtual() = delete;
		ZAllocator_SEMalloc_Virtual(const ZAllocator_SEMalloc_Virtual&) = delete;
		ZAllocator_SEMalloc_Virtual(ZAllocator_SEMalloc_Virtual&&) = delete;
		static void* allocate(size_t sz);
		static void* alignedAllocate(size_t sz, size_t alignment);
		static void deallocate(void* ptr);
		static void* calloc(size_t nmemb, size_t size);
		static void* calloc(size_t nmemb, size_t size, size_t alignment);
		static void* realloc(void* ptr, size_t new_size);
		static void* realloc(void* ptr, size_t new_size, size_t alignment);
		static void releaseAllArenas();
	private:
		// ---------- Internal Allocation Methods ----------------------------------------------
		static void* allocateFromCachedPool(size_t sz, size_t alignment);
		static void* allocateFromSystem(size_t sz, size_t alignment);

	private:
		// ---------- Size and Alignment Class Helpers -----------------------------------------
		struct InternalSizeAlignmentClassHelpers {
			static constexpr int32_t getClassFromSize(size_t sz) {
				return ::StateEngine::EngineCore::MemoryManagment::getClassFromSize(
					sz,
					kLargeAllocThreshold,
					kPowerOfTwoUpperBound,
					kPowerOfTwoLowerBound
				);
			}
			static constexpr size_t getSizeFromClass(int32_t cls) {
				return ::StateEngine::EngineCore::MemoryManagment::getSizeFromClass(
					cls,
					kLargeAllocThreshold,
					kPowerOfTwoUpperBound,
					kPowerOfTwoLowerBound
				);
			}
			static constexpr int32_t getClassFromAlignment(size_t alignment) {
				return ::StateEngine::EngineCore::MemoryManagment::getClassFromAlignment(alignment);
			}
			static constexpr size_t getAlignmentFromClass(int32_t cls){
				return ::StateEngine::EngineCore::MemoryManagment::getAlignmentFromClass(cls);
			}
		};

	private:
		// ---------- Memory Chunk and Arena Headers -------------------------------------------
		struct MemoryChunkHeader{
			MemoryChunkHeader* nextFree;
			MemoryChunkHeader* nextRemoteFree;
			uint16_t spanItemIndex;
			uint8_t alignment;
			uint32_t payloadSize;
			size_t magic;
		};
		struct MemoryArenaInfo {
			MemoryArenaInfo* nextThreadLocalArena;
			MemoryArenaInfo* nextGlobalArena;
			std::atomic<MemoryChunkHeader*> remoteFreeListHead;
			std::atomic<uint32_t> livedChunkCount;
			bool isLargeAllocArena;
			std::atomic<bool> orphaned;
			size_t payloadSize;
			void* basePtr;
			size_t reservedSize;
			size_t committedSize;
			void* freeDataPtr;
			size_t magic;
		};
		static MemoryArenaInfo* allArenasHead_;
		static std::shared_mutex allArenasMutex_;
		inline static void removeArenaFromGlobalList(MemoryArenaInfo* arena) {
			std::unique_lock<std::shared_mutex> lock(allArenasMutex_);

			if (!allArenasHead_ || !arena) {
				return;
			}

			if (allArenasHead_ == arena) {
				allArenasHead_ = allArenasHead_->nextGlobalArena;
				return;
			}

			MemoryArenaInfo* current = allArenasHead_;
			while (current->nextGlobalArena != nullptr && current->nextGlobalArena != arena) {
				current = current->nextGlobalArena;
			}

			if (current->nextGlobalArena == arena) {
				current->nextGlobalArena = arena->nextGlobalArena;
			}
		}
		inline static void addArenaToGlobalList(MemoryArenaInfo* arena) {
			std::unique_lock<std::shared_mutex> lock(allArenasMutex_);

			arena->nextGlobalArena = allArenasHead_;

			allArenasHead_ = arena;
		}

		// ---------- Chunk Span Management --------------------------------------------------
		template<size_t ChunksCount>
		class ChunkSpan {
		private:
			MemoryArenaInfo* ownedArena_;
			std::thread::id ownedThread_;
			size_t chunkSize_;
			size_t alignment_;
			size_t chunkStride_;
			ZAtomicBitset<ChunksCount> used_;
		public:
			ChunkSpan(size_t chunkSize, size_t alignment,
				MemoryArenaInfo* ownedArena, std::thread::id ownedThread)
				: ownedArena_(ownedArena),
				  ownedThread_(ownedThread),
				  chunkSize_(chunkSize),
				  alignment_(alignment)
			{
				chunkStride_ = align_up(chunkSize + sizeof(MemoryChunkHeader) + alignment, alignment_);
				used_.clear();

				for(size_t i = 0; i < ChunksCount; ++i) {
					auto header = chunkHeader(i);
					header->nextFree = nullptr;
					header->nextRemoteFree = nullptr;
					header->spanItemIndex = static_cast<uint16_t>(i);
					header->alignment = static_cast<uint8_t>(alignment_);
					header->payloadSize = static_cast<uint32_t>(chunkSize_);
					header->magic = kChunkHeaderMagic;
				}
			}
			inline size_t getChunksCount() const {
				return ChunksCount;
			}
			inline MemoryArenaInfo* getOwnedArena() const {
				return ownedArena_;
			}
			inline std::thread::id getOwnedThread() const {
				return ownedThread_;
			}
			inline MemoryChunkHeader* getAt(size_t i) {
				if (i >= ChunksCount) return nullptr;
				return chunkHeader(i);
			}
			inline MemoryChunkHeader* getFree() {
				for (size_t i = 0; i < ChunksCount; ++i) {
					if (!used_.test(i))
						return chunkHeader(i);
				}
				return nullptr;
			}
			inline void markUsed(size_t i) {
				if (i >= ChunksCount) return;
				used_.set(i);
			}
			inline void markFree(size_t i) {
				if (i >= ChunksCount) return;
				used_.reset(i);
			}
			inline bool isUsed(size_t i) const {
				if (i >= ChunksCount) return false;
				return used_.test(i);
			}
			inline static size_t totalBytes(size_t chunkSize, size_t alignment) {
				const size_t stride = align_up(chunkSize + sizeof(MemoryChunkHeader) + alignment, alignment);
				return sizeof(ChunkSpan<ChunksCount>) + stride * ChunksCount;
			}
			inline static ChunkSpan<ChunksCount>* getSpanFromChunk(MemoryChunkHeader* chunk) {
				if (!chunk) return nullptr;
				auto base = reinterpret_cast<uintptr_t>(chunk) - (chunk->spanItemIndex * align_up(chunk->payloadSize + sizeof(MemoryChunkHeader) + chunk->alignment, chunk->alignment)) - sizeof(ChunkSpan<ChunksCount>);
				return reinterpret_cast<ChunkSpan<ChunksCount>*>(base);
			}
		private:
			inline MemoryChunkHeader* chunkHeader(size_t i) {
				auto base = reinterpret_cast<uintptr_t>(this) + sizeof(*this);
				return reinterpret_cast<MemoryChunkHeader*>(base + i * chunkStride_);
			}
		};
		using ChunkSpan64 = ChunkSpan<64>;
		using ChunkSpan32 = ChunkSpan<32>;
		using ChunkSpan16 = ChunkSpan<16>;
		using ChunkSpan1 = ChunkSpan<1>;
		using AnyChunkSpan = std::variant<ChunkSpan64*, ChunkSpan32*, ChunkSpan16*, ChunkSpan1*, std::nullptr_t>;
		static constexpr uint32_t getChunkSpanClassFromSize(size_t sz) {
			if (sz <= 64) return 0;
			else if (sz <= 512) return 1;
			else if (sz <= 4096) return 2;
			else return 3;
		}
		
		// ---------- Thread Local Control Block ---------------------------------------------
		class ThreadLocalControlBlock {
		private:
			static constexpr int32_t sizeClassFreeChunkHeadsCount = getClassFromSize(
				kLargeAllocThreshold - 1,
				kLargeAllocThreshold,
				kPowerOfTwoUpperBound,
				kPowerOfTwoLowerBound
			) + 1;
			MemoryChunkHeader* sizeClassFreeChunkHeads_[sizeClassFreeChunkHeadsCount][kSupportedAlignmentsCount]{ nullptr };
			MemoryArenaInfo* memoryArenasHead_{ nullptr };
		public:
			static constexpr size_t kDrainAfterAllocationAmount = 32;
			static constexpr size_t kMaxReserveSizePerThread = 256 * 1024 * 1024; //256 MB
			static constexpr size_t kMaxCommitSizePerThread = 64 * 1024 * 1024;   //64 MB
			size_t drainAllocationAmountCount_{ 0 };
			size_t memoryReserveSize_{ 4 * 1024 * 1024 }; // 4 MB
			size_t memoryCommitSize_{ 1024 * 1024 }; // 1 MB
		public:
			inline void pushFreeChunk(MemoryChunkHeader* chunk) {
				if(!chunk)
					return; // Null chunk header
				int32_t szCls = InternalSizeAlignmentClassHelpers::getClassFromSize(chunk->payloadSize);
				int32_t alignCls = InternalSizeAlignmentClassHelpers::getClassFromAlignment(chunk->alignment);
				if(szCls < 0 || alignCls < 0)
					return; // Invalid chunk header
				chunk->nextFree = sizeClassFreeChunkHeads_[szCls][alignCls];
				sizeClassFreeChunkHeads_[szCls][alignCls] = chunk;
			}
			inline MemoryChunkHeader* popFreeChunk(int32_t szCls, int32_t alignCls) {
				if(szCls < 0 || alignCls < 0)
					return nullptr; // Invalid class
				MemoryChunkHeader* head = sizeClassFreeChunkHeads_[szCls][alignCls];
				if(!head)
					return nullptr; // No free chunk
				sizeClassFreeChunkHeads_[szCls][alignCls] = head->nextFree;
				return head;
			}
			inline void pushMemoryArena(MemoryArenaInfo* arena) {
				if(!arena)
					return; // Null arena
				arena->nextThreadLocalArena = memoryArenasHead_;
				memoryArenasHead_ = arena;
				
				// Also push to global list
				addArenaToGlobalList(arena);
			}
			inline bool removeMemoryArena(MemoryArenaInfo* arena) {
				if (!arena || !memoryArenasHead_)
					return false; // Null arena or empty list
				// Remove from thread-local list
				if (memoryArenasHead_ == arena) {
					memoryArenasHead_ = arena->nextThreadLocalArena;
				}
				else {
					MemoryArenaInfo* prev = memoryArenasHead_;
					while (prev && prev->nextThreadLocalArena != arena) {
						prev = prev->nextThreadLocalArena;
					}
					if (prev) {
						prev->nextThreadLocalArena = arena->nextThreadLocalArena;
					}
				}
				// Remove from global list
				removeArenaFromGlobalList(arena);
				return true;
			}
			inline MemoryArenaInfo* getMemoryArenasHead() const {
				return memoryArenasHead_;
			}
			~ThreadLocalControlBlock() {
				MemoryArenaInfo* curArena = memoryArenasHead_;
				while (curArena)
				{
					curArena->orphaned.store(true, std::memory_order_release);
					uint32_t livedChunks = curArena->livedChunkCount.load(std::memory_order_relaxed);
					MemoryArenaInfo* curNext = curArena->nextThreadLocalArena;
					if (!livedChunks) {
						removeMemoryArena(curArena);
						PageAllocator::tryRelease(curArena->basePtr);
					}
					curArena = curNext;
				}
			}
		};
		static thread_local ThreadLocalControlBlock threadLocalControlBlock_;

	private:
		// ---------- Remote Free List Helpers -----------------------------------------------
		inline static void pushRemoteFree(MemoryArenaInfo* arena, MemoryChunkHeader* chunk) {
			MemoryChunkHeader* oldHead = arena->remoteFreeListHead.load(std::memory_order_relaxed);
			do {
				chunk->nextRemoteFree = oldHead;
			} while (!arena->remoteFreeListHead.compare_exchange_weak(oldHead, chunk,
				std::memory_order_release, std::memory_order_relaxed));
		}
		inline static MemoryChunkHeader* drainRemoteFrees(MemoryArenaInfo* arena) {
			return arena->remoteFreeListHead.exchange(nullptr, std::memory_order_acquire);
		}

	private:
		static bool allocateNewArena();
		static size_t tryExpadArena(MemoryArenaInfo* arena, size_t needSize);
		static AnyChunkSpan tryCraveSpanInArena(MemoryArenaInfo* arena, size_t chunkSize, size_t alignment);
		static AnyChunkSpan ñraveSpanAnyArena(size_t chunkSize, size_t alignment);
		constexpr static size_t align_up(size_t value, size_t alignment) {
			return (value + alignment - 1) & ~(alignment - 1);
		}
		inline static bool isValidAddress(void* ptr) {
			if (!ptr) return false;
			MemoryArenaInfo* arena = threadLocalControlBlock_.getMemoryArenasHead();
			uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
			while (arena)
			{
				uintptr_t arenaStart = reinterpret_cast<uintptr_t>(arena->basePtr);
				uintptr_t arenaEnd = arenaStart + arena->committedSize;
				if (addr > arenaStart && addr < arenaEnd)
					return true;
				arena = arena->nextThreadLocalArena;
			}
			std::shared_lock<std::shared_mutex> lock(allArenasMutex_);
			arena = allArenasHead_;
			while (arena) {
				uintptr_t arenaStart = reinterpret_cast<uintptr_t>(arena->basePtr);
				uintptr_t arenaEnd = arenaStart + arena->committedSize;
				if (addr > arenaStart && addr < arenaEnd)
					return true;
				arena = arena->nextGlobalArena;
			}
			return false;
		}
	};
}