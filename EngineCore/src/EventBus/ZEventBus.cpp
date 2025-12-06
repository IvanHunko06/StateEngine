#include "ZEventBus.hpp"
#include "EngineTypeSystem/ZTypeRegistry.hpp"
#include "EngineCore/EngineTypeSystem/ReflectionMacros.hpp"
#include "EngineCore/BaseEngineEvents/ShutdownEngineEvent.hpp"


using namespace StateEngine::EngineCore::EventBus;
using namespace StateEngine::EngineCore::BaseEngineEvents;
using namespace StateEngine::EngineCore::EngineTypeSystem;
using StateEngine::EngineCore::EngineTypeSystem::ZTypeRegistry;


ZHashMap<const TypeInfo*, ZBuffer<EventCallback>> ZEventBus::eventsCallbacks_;
MPMCQueue<ZEventBus::EventPublishTask> ZEventBus::eventsQueue_;
MPMCQueue<ZEventBus::SubscriptionCommand> ZEventBus::subscriptionQueue_;
std::array<ZFrameAllocator, 2> ZEventBus::eventAllocators_{ {
	ZFrameAllocator(ZEventBus::kEventFixedAllocatorSize),
	ZFrameAllocator(ZEventBus::kEventFixedAllocatorSize)
} };
std::atomic<uint8_t> ZEventBus::activeWriteIndex_;

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
	if (!eventType) {
		assert(false && "Invalid eventType in ZEventBus::Publish");
		return;
	}
	if (!userdata) {
		eventsQueue_.enqueue(EventPublishTask{
			.eventType = eventType,
			.eventData = nullptr
		});
		return;
	}
	// Allocate event data in the active allocator
	uint8_t writeIndex = activeWriteIndex_.load(std::memory_order_relaxed);
	auto& allocator = eventAllocators_[writeIndex];

	void* eventDataMemory = allocator.allocate(eventType->size, eventType->alignment);
	if(!eventDataMemory) {
		assert(false && "Failed to allocate event data memory in ZEventBus::Publish");
		ZLOG_ERROR("EventBus") << "Failed to allocate event data memory for event: " << eventType->name.c_str();
		return;
	}
	// Copy construct the event data
	if(eventType->copyConstructor) {
		eventType->copyConstructor(eventDataMemory, userdata);
	} else {
		std::memcpy(eventDataMemory, userdata, eventType->size);
	}
	
	eventsQueue_.enqueue(EventPublishTask{
		.eventType = eventType,
		.eventData = eventDataMemory
	});
}
void ZEventBus::RegisterBaseEvents() {
	BEGIN_REFLECT_STRUCT("ShutdownEngineEvent", ShutdownEngineEvent);
	END_REFLECT
}
void ZEventBus::FlushEvents() {
	ProcessSubscriptionCommands();
	uint8_t readIndex = activeWriteIndex_.load(std::memory_order_relaxed);
	uint8_t writeIndex = (readIndex + 1) % 2;
	activeWriteIndex_.store(writeIndex, std::memory_order_relaxed);

	EventPublishTask buffer[kMaxBulkEventProcessCount];
	size_t count;
	while ((count = eventsQueue_.try_dequeue_bulk(buffer, kMaxBulkEventProcessCount)) != 0) {
		for (size_t i = 0; i < count; ++i) {
			auto& curTask = buffer[i];

			if (!eventsCallbacks_.contains(curTask.eventType)) continue;
			ZBuffer<EventCallback>& eventCallbacks = eventsCallbacks_[curTask.eventType];

			for (auto& callback : eventCallbacks) {
				bool consumed = callback(curTask.eventData);
				if (consumed) break;
			}
		}
	}

	// Clear the read allocator
	eventAllocators_[readIndex].clear();
}

void ZEventBus::ProcessSubscriptionCommands() {
	SubscriptionCommand buffer[kMaxBulkEventProcessCount];
	size_t count;
	while ((count = subscriptionQueue_.try_dequeue_bulk(buffer, kMaxBulkEventProcessCount)) != 0) {
		for (size_t i = 0; i < count; ++i) {
			auto& cmd = buffer[i];
			if (cmd.action == SubscriptionAction::Subscribe) {
				if (eventsCallbacks_.contains(cmd.eventType)) {
					eventsCallbacks_[cmd.eventType].push_back(cmd.callback);
					continue;
				}
				ZBuffer<EventCallback> callbacksBuffer;
				callbacksBuffer.push_back(cmd.callback);
				eventsCallbacks_[cmd.eventType] = std::move(callbacksBuffer);
			}
			else if (cmd.action == SubscriptionAction::Unsubscribe) {
				if (!eventsCallbacks_.contains(cmd.eventType)) {
					continue;
				}
				auto& callbacksBuffer = eventsCallbacks_[cmd.eventType];
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