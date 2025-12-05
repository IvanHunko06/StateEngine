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
	public:
		enum class SubscriptionAction : uint8_t {
			Subscribe,
			Unsubscribe
		};
		struct SubscriptionCommand {
			SubscriptionAction action;
			const TypeInfo* eventType;
			EventCallback callback;
		};
	private:
		static ZHashMap<const TypeInfo*, ZBuffer<EventCallback>> eventCallbacks_;
		struct EventPublishTask {
			const TypeInfo* eventType{ nullptr };
			TypeInstance instance;
			EventPublishTask() = default;
			EventPublishTask(const TypeInstance& instance)
				: eventType(instance.getTypeInfo()), instance(instance) {}
		};
		static MPMCQueue<EventPublishTask> eventsQueue_;
		static MPMCQueue<SubscriptionCommand> subscriptionQueue_;
		static constexpr size_t kMaxBulkEventProcessCount = 128;

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