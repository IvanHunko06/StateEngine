#pragma once
#include "EngineCore/EventBus/EventCallback.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/EngineTypeSystem/TypeInstance.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/SmartPointers/ZUniquePointer.hpp"
#include "EngineCore/Threading/MPMCQueue.hpp"
#include "EngineCore/MemoryManagment/ZFrameAllocator.hpp"
using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::EngineCore::DataStructures;
using StateEngine::EngineCore::EventBus::EventCallback;
using StateEngine::EngineCore::SmartPointers::ZUniquePointer;
using StateEngine::EngineCore::Threading::MPMCQueue;
using StateEngine::EngineCore::MemoryManagment::ZFrameAllocator;



namespace StateEngine::EngineCore::EventBus {
	class ZEventBus {
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
		static std::array<ZFrameAllocator, 2> eventAllocators_;
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