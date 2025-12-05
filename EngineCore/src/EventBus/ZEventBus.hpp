#pragma once
#include "EngineCore/EventBus/EventCallback.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/EngineTypeSystem/TypeInstance.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/SmartPointers/ZUniquePointer.hpp"
#include "EngineCore/Threading/MPMCQueue.hpp"
using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::EngineCore::DataStructures;
using StateEngine::EngineCore::EventBus::EventCallback;
using StateEngine::EngineCore::SmartPointers::ZUniquePointer;
using StateEngine::EngineCore::Threading::MPMCQueue;



namespace StateEngine::EngineCore::EventBus {
	class ZEventBus {
	private:
        class EventFixedAllocator {
        private:
            void* memory_{ nullptr };
            size_t capacity_{ 0 };
            std::atomic<size_t> allocated_{ 0 };
            MPMCQueue<void*> fallbackAllocations_;

        public:
            EventFixedAllocator(size_t capacity) : capacity_(capacity) {
                memory_ = MemoryAllocator_AlignedAllocate(capacity, 64);
                allocated_.store(0, std::memory_order_relaxed);
            }

            ~EventFixedAllocator() {
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
				ZLOG_DEBUG("EngineCore") << "EventFixedAllocator: Falling back to heap allocation for size " << size << " and alignment " << alignment;
                void* ptr = MemoryAllocator_AlignedAllocate(size, alignment);
                if (ptr) {
                    fallbackAllocations_.enqueue(ptr);
                }
                return ptr;
            }
        };
	private:
		enum class SubscriptionAction : uint8_t {
			Subscribe,
			Unsubscribe
		};
		struct SubscriptionCommand {
			SubscriptionAction action;
			const TypeInfo* eventType;
			EventCallback callback;
		};
        struct EventPublishTask {
            const TypeInfo* eventType{ nullptr };
            void* eventData{ nullptr };
        };
	private:
		static ZHashMap<const TypeInfo*, ZBuffer<EventCallback>> eventsCallbacks_;
		static MPMCQueue<EventPublishTask> eventsQueue_;
		static MPMCQueue<SubscriptionCommand> subscriptionQueue_;
		static constexpr size_t kMaxBulkEventProcessCount = 128;
		static constexpr size_t kEventFixedAllocatorSize = 2 * 1024 * 1024; // 2 MB
		static std::array<EventFixedAllocator, 2> eventAllocators_;
		static std::atomic<uint8_t> activeWriteIndex_;

	public:
		static void Subscribe(const TypeInfo* eventType, const EventCallback& callback);
		static void Unsubscribe(const TypeInfo* eventType, const EventCallback& callback);
		static void Publish(const TypeInfo* eventType, void* userdata);
		static void FlushEvents();
		static void RegisterBaseEvents();
	private:
		static void ProcessSubscriptionCommands();
	};
}