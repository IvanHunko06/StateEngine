#include "ZEventBus.hpp"
#include "EngineTypeSystem/ZTypeRegistry.hpp"
#include "EngineCore/EngineTypeSystem/ReflectionMacros.hpp"
#include "EngineCore/BaseEngineEvents/ShutdownEngineEvent.hpp"


using namespace StateEngine::EngineCore::EventBus;
using namespace StateEngine::EngineCore::BaseEngineEvents;
using namespace StateEngine::EngineCore::EngineTypeSystem;
using StateEngine::EngineCore::EngineTypeSystem::ZTypeRegistry;


ZHashMap<const TypeInfo*, ZBuffer<EventCallback>> ZEventBus::eventCallbacks_;
MPMCQueue<ZEventBus::EventPublishTask> ZEventBus::eventsQueue_;
MPMCQueue<ZEventBus::SubscriptionCommand> ZEventBus::subscriptionQueue_;

void ZEventBus::Subscribe(const TypeInfo* eventType, const EventCallback& callback) {
	if (!eventType || !callback) {
		assert(false && "Invalid eventType or callback in ZEventBus::Subscribe");
		return;
	}

	SubscriptionCommand cmd{
		.action = SubscriptionAction::Subscribe,
		.eventType = eventType,
		.callback = callback
	};
	subscriptionQueue_.enqueue(cmd);
}
void ZEventBus::Unsubscribe(const TypeInfo* eventType, const EventCallback& callback) {
	if (!eventType || !callback) {
		assert(false && "Invalid eventType or callback in ZEventBus::Unsubscribe");
		return;
	}
	SubscriptionCommand cmd{
		.action = SubscriptionAction::Unsubscribe,
		.eventType = eventType,
		.callback = callback
	};
	subscriptionQueue_.enqueue(cmd);
}
void ZEventBus::Publish(const TypeInfo* eventType, void* userdata) {
	eventsQueue_.enqueue(TypeInstance(userdata, eventType));
}
void ZEventBus::RegisterBaseEvents() {
	BEGIN_REFLECT_STRUCT("ShutdownEngineEvent", ShutdownEngineEvent);
	END_REFLECT
}
void ZEventBus::FlushEvents() {
	ProcessSubscriptionCommands();
	EventPublishTask buffer[kMaxBulkEventProcessCount];
	size_t count;
	while ((count = eventsQueue_.try_dequeue_bulk(buffer, kMaxBulkEventProcessCount)) != 0) {
		for (size_t i = 0; i < count; ++i) {
			auto& curTask = buffer[i];

			if (!eventCallbacks_.contains(curTask.eventType)) continue;
			ZBuffer<EventCallback>& callbacksCopy = eventCallbacks_[curTask.eventType];

			for (auto& callback : callbacksCopy) {
				bool consumed = callback(curTask.instance.getRawPtr());
				if (consumed) break;
			}
		}
	}
}

void ZEventBus::ProcessSubscriptionCommands() {
	SubscriptionCommand buffer[kMaxBulkEventProcessCount];
	size_t count;
	while ((count = subscriptionQueue_.try_dequeue_bulk(buffer, kMaxBulkEventProcessCount)) != 0) {
		for (size_t i = 0; i < count; ++i) {
			auto& cmd = buffer[i];
			if (cmd.action == SubscriptionAction::Subscribe) {
				if (eventCallbacks_.contains(cmd.eventType)) {
					eventCallbacks_[cmd.eventType].push_back(cmd.callback);
					continue;
				}
				ZBuffer<EventCallback> callbacksBuffer;
				callbacksBuffer.push_back(cmd.callback);
				eventCallbacks_[cmd.eventType] = std::move(callbacksBuffer);
			}
			else if (cmd.action == SubscriptionAction::Unsubscribe) {
				if (!eventCallbacks_.contains(cmd.eventType)) {
					continue;
				}
				auto& callbacksBuffer = eventCallbacks_[cmd.eventType];
				for (size_t j = 0; j < callbacksBuffer.size(); ++j) {
					if (callbacksBuffer[j] == cmd.callback) {
						callbacksBuffer.erase(callbacksBuffer.begin() + j);
						break;
					}
				}
			}
		}
	}
}