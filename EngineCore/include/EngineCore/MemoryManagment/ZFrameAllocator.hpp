#pragma once
#include "EngineCore/Threading/MPMCQueue.hpp"
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"

using StateEngine::EngineCore::Threading::MPMCQueue;
namespace StateEngine::EngineCore::MemoryManagment {
    class ZFrameAllocator {
    private:
        void* memory_{ nullptr };
        size_t capacity_{ 0 };
        std::atomic<size_t> allocated_{ 0 };
        MPMCQueue<void*> fallbackAllocations_;
        const char* name_{ nullptr };

    public:
        ZFrameAllocator(size_t capacity, const char* name = nullptr) : 
            capacity_(capacity),
            name_(name)
        {
            memory_ = MemoryAllocator_AlignedAllocate(capacity, 64);
            allocated_.store(0, std::memory_order_relaxed);
        }
        

        ~ZFrameAllocator() {
            clear();

            if (memory_) {
                MemoryAllocator_Deallocate(memory_);
            }
        }

        void* allocate(size_t size, size_t alignment) {
            if (alignment > 16) {
                return allocateFallback(size, alignment);
            }

            size_t alignedSize = (size + 15) & ~15;

            size_t offset = allocated_.fetch_add(alignedSize, std::memory_order_acquire);

            if (offset + alignedSize > capacity_) {
                return allocateFallback(size, alignment);
            }

            return static_cast<uint8_t*>(memory_) + offset;
        }

        void clear() {
            allocated_.store(0, std::memory_order_release);
            void* fallbackMemory;
            while (fallbackAllocations_.try_dequeue(fallbackMemory)) {
                MemoryAllocator_Deallocate(fallbackMemory);
            }
        }

    private:
        void* allocateFallback(size_t size, size_t alignment) {
            ZLOG_WARN("ZFrameAllocator") 
                << "Fallback allocation of size " << size 
                << " and alignment " << alignment
                << " for pool allocator " << (name_ ? name_ : "");
            void* ptr = MemoryAllocator_AlignedAllocate(size, alignment);
            if (ptr) {
                fallbackAllocations_.enqueue(ptr);
            }
            return ptr;
        }
    };
}