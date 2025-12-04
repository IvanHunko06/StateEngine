#include "ZEventBus.hpp"
#include "EngineTypeSystem/ZTypeRegistry.hpp"
#include "EngineCore/EngineTypeSystem/ReflectionMacros.hpp"
#include "EngineCore/BaseEngineEvents/ShutdownEngineEvent.hpp"


using namespace StateEngine::EngineCore::EventBus;
using namespace StateEngine::EngineCore::BaseEngineEvents;
using namespace StateEngine::EngineCore::EngineTypeSystem;
using StateEngine::EngineCore::EngineTypeSystem::ZTypeRegistry;


ZHashMap<const TypeInfo*, ZBuffer<EventCallback>> ZEventBus::eventCallbacks_;
std::shared_mutex ZEventBus::eventBusMutex_;
MPMCQueue<ZEventBus::EventPublishTask> ZEventBus::eventsQueue_;

void ZEventBus::Subscribe(const TypeInfo* eventType, const EventCallback& callback) {
	if (!eventType || !callback) {
		return;
	}
	std::unique_lock<std::shared_mutex> lock(eventBusMutex_);
	if (eventCallbacks_.contains(eventType)) {
		eventCallbacks_[eventType].push_back(callback);
		return;
	}
	ZBuffer<EventCallback> callbacksBuffer;
	callbacksBuffer.push_back(callback);
	eventCallbacks_[eventType] = std::move(callbacksBuffer);
}
void ZEventBus::Unsubscribe(const TypeInfo* eventType, const EventCallback& callback) {
	if (!eventType || !callback) {
		return;
	}
	std::unique_lock<std::shared_mutex> lock(eventBusMutex_);
	if (!eventCallbacks_.contains(eventType)) {
		return;
	}
	auto& callbacksBuffer = eventCallbacks_[eventType];
	for (size_t i = 0; i < callbacksBuffer.size(); ++i) {
		if (callbacksBuffer[i] == callback) {
			callbacksBuffer.erase(callbacksBuffer.begin() +i);
			return;
		}
	}
}
void ZEventBus::Publish(const TypeInfo* eventType, void* userdata) {
	eventsQueue_.enqueue(TypeInstance(userdata, eventType));
}
void ZEventBus::RegisterBaseEvents() {
	BEGIN_REFLECT_STRUCT("ShutdownEngineEvent", ShutdownEngineEvent);
	END_REFLECT
}
void ZEventBus::FlushEvents() {
	ZHashMap<const TypeInfo*, ZBuffer<EventCallback>> callbacksSnapshot;
	EventPublishTask buffer[kMaxBulkEventProcessCount];
	size_t count;
	while ((count = eventsQueue_.try_dequeue_bulk(buffer, kMaxBulkEventProcessCount)) != 0) {
		for (size_t i = 0; i < count; ++i) {
			auto& curTask = buffer[i];

			ZBuffer<EventCallback> callbacksCopy;
			{
				std::shared_lock<std::shared_mutex> lock(eventBusMutex_);
				if (!eventCallbacks_.contains(curTask.eventType)) continue;
				callbacksCopy = eventCallbacks_[curTask.eventType];
			}

			for (auto& callback : callbacksCopy) {
				bool consumed = callback(curTask.instance.getRawPtr());
				if (consumed) break;
			}
		}
	}
}