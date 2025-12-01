#include "ZEventBus.hpp"
#include "EngineTypeSystem/ZTypeRegistry.hpp"
#include "EngineCore/EngineTypeSystem/ReflectionMacros.hpp"
#include "EngineCore/BaseEngineEvents/ShutdownEngineEvent.hpp"


using namespace StateEngine::EngineCore::EventBus;
using namespace StateEngine::EngineCore::BaseEngineEvents;
using namespace StateEngine::EngineCore::EngineTypeSystem;
using StateEngine::EngineCore::EngineTypeSystem::ZTypeRegistry;


ZHashMap<const TypeInfo*, ZBuffer<ZEventBus::EventListenerCallbackContext>> ZEventBus::eventCallbacks_;
std::shared_mutex ZEventBus::eventBusMutex_;
MPMCQueue<ZEventBus::EventPublishTask> ZEventBus::eventsQueue_;

void ZEventBus::subscribe(const TypeInfo* eventType, void* listener, EventCallback callback) {
	if (!eventType || !listener || !callback) {
		return;
	}
	std::unique_lock<std::shared_mutex> lock(eventBusMutex_);
	EventListenerCallbackContext context{
		.callback = callback,
		.listener = listener
	};
	if (eventCallbacks_.contains(eventType)) {
		eventCallbacks_[eventType].push_back(context);
		return;
	}
	ZBuffer<EventListenerCallbackContext> callbacksBuffer;
	callbacksBuffer.push_back(context);
	eventCallbacks_[eventType] = std::move(callbacksBuffer);
}
void ZEventBus::unsubscribe(const TypeInfo* eventType, void* listener, EventCallback callback) {
	if (!eventType || !listener || !callback) {
		return;
	}
	std::unique_lock<std::shared_mutex> lock(eventBusMutex_);
	if (!eventCallbacks_.contains(eventType)) {
		return;
	}
	auto& callbacksBuffer = eventCallbacks_[eventType];
	for (size_t i = 0; i < callbacksBuffer.size(); ++i) {
		if (callbacksBuffer[i].callback == callback && callbacksBuffer[i].listener == listener) {
			callbacksBuffer.erase(callbacksBuffer.begin() +i);
			return;
		}
	}
}
void ZEventBus::publish(const TypeInfo* eventType, void* userdata) {
	eventsQueue_.enqueue(TypeInstance(userdata, eventType));
}
void ZEventBus::registerBaseEvents() {
	BEGIN_REFLECT_STRUCT("ShutdownEngineEvent", ShutdownEngineEvent);
	END_REFLECT
}
void ZEventBus::flushEvents() {
	ZHashMap<const TypeInfo*, ZBuffer<EventListenerCallbackContext>> callbacksSnapshot;
	EventPublishTask buffer[kMaxBulkEventProcessCount];
	size_t count;
	while ((count = eventsQueue_.try_dequeue_bulk(buffer, kMaxBulkEventProcessCount)) != 0) {
		for (size_t i = 0; i < count; ++i) {
			/*auto& curTask = buffer[i];
			auto curListenersValue = callbacksSnapshot.getValue(curTask.eventType);
			if(curListenersValue.has_value()){
				auto& callbacksCopy = curListenersValue->get();
				for (auto& callbackContext : callbacksCopy) {
					bool consumed = callbackContext.callback(callbackContext.listener, curTask.userDataWrapper.getUserData());
					if (consumed) break;
				}
			}
			else {
				ZBuffer<EventListenerCallbackContext> callbacksCopy;
				{
					std::shared_lock<std::shared_mutex> lock(eventBusMutex_);
					auto curListenersValue = eventCallbacks_.getValue(curTask.eventType);
					if (!curListenersValue.has_value()) {
						continue;
					}
					callbacksCopy = curListenersValue->get();
				}
				for (auto& callbackContext : callbacksCopy) {
					bool consumed = callbackContext.callback(callbackContext.listener, curTask.userDataWrapper.getUserData());
					if (consumed) break;
				}
				callbacksSnapshot.insert(curTask.eventType, std::move(callbacksCopy));
			}*/
			auto& curTask = buffer[i];

			ZBuffer<EventListenerCallbackContext> callbacksCopy;
			{
				std::shared_lock<std::shared_mutex> lock(eventBusMutex_);
				if (!eventCallbacks_.contains(curTask.eventType)) continue;
				callbacksCopy = eventCallbacks_[curTask.eventType];
			}

			for (auto& callbackContext : callbacksCopy) {
				bool consumed = callbackContext.callback(callbackContext.listener, curTask.instance.getRawPtr());
				if (consumed) break;
			}
		}
	}
}