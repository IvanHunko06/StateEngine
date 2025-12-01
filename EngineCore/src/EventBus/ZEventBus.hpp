#pragma once
#include "EngineCore/EventBus/EventCallback.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/EngineTypeSystem/TypeInstance.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/SmartPointers/ZUniquePointer.hpp"
#include "EngineCore/Threading/MPMCQueue.hpp"

#include <shared_mutex>
using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::EngineCore::DataStructures;
using StateEngine::EngineCore::EventBus::EventCallback;
using StateEngine::EngineCore::SmartPointers::ZUniquePointer;
using StateEngine::EngineCore::Threading::MPMCQueue;


namespace StateEngine::EngineCore::EventBus {
	class ZEventBus {
	private:
		struct EventListenerCallbackContext {
			EventCallback callback;
			void* listener;
		};
		static ZHashMap<const TypeInfo*, ZBuffer<EventListenerCallbackContext>> eventCallbacks_;
		static std::shared_mutex eventBusMutex_;
		struct EventPublishTask {
			const TypeInfo* eventType{ nullptr };
			TypeInstance instance;
			EventPublishTask() = default;
			EventPublishTask(const TypeInstance& instance)
				: eventType(instance.getTypeInfo()), instance(instance) {}
		};
		static MPMCQueue<EventPublishTask> eventsQueue_;
		static constexpr size_t kMaxBulkEventProcessCount = 128;

	public:
		static void subscribe(const TypeInfo* eventType, void* listener, EventCallback callback);
		static void unsubscribe(const TypeInfo* eventType, void* listener, EventCallback callback);
		static void publish(const TypeInfo* eventType, void* userdata);
		static void flushEvents();
		static void registerBaseEvents();
	};
}