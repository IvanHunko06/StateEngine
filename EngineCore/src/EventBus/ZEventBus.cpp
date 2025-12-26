#include "ZEventBus.hpp"
#include "EngineCore/BaseEngineEvents/ShutdownEngineEvent.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/EngineTypeSystem/CompileTimeTypeBuilder.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistry.hpp"
#include "EngineCore/EventBus/IEventBus.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>

using namespace StateEngine::EngineCore::EventBus;
using namespace StateEngine::EngineCore::BaseEngineEvents;
using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::EngineCore::DataStructures;

EventSubscriptionHandle ZEventBus::Subscribe(const TypeInfo* eventType, EventCallback&& callback)
{
    if (eventType == nullptr || !callback) {
        assert(false && "Invalid eventType or callback in ZEventBus::Subscribe");
        return 0;
    }
    const EventSubscriptionHandle handle = CurrentEventSubscriptionHandle.fetch_add(1);
    const SubscriptionCommand cmd {.action             = SubscriptionAction::Subscribe,
                                   .eventType          = eventType,
                                   .callback           = std::move(callback),
                                   .subscriptionHandle = handle};
    SubscriptionQueue.enqueue(cmd);
    return handle;
}
void ZEventBus::Unsubscribe(const TypeInfo* eventType, EventSubscriptionHandle listenerHandle)
{
    if (eventType == nullptr || listenerHandle == 0) {
        assert(false && "Invalid eventType or callback in ZEventBus::Unsubscribe");
        return;
    }
    const SubscriptionCommand cmd {
        .action = SubscriptionAction::Unsubscribe, .eventType = eventType, .subscriptionHandle = listenerHandle};
    SubscriptionQueue.enqueue(cmd);
}
void ZEventBus::Publish(const TypeInfo* eventType, const void* eventData)
{
    if (eventType == nullptr) {
        assert(false && "Invalid eventType in ZEventBus::Publish");
        return;
    }
    if (eventData == nullptr) {
        EventsQueue.enqueue(EventPublishTask {.eventType = eventType, .eventData = nullptr});
        return;
    }
    // Allocate event data in the active allocator
    const uint8_t writeIndex = ActiveWriteIndex.load(std::memory_order_relaxed);
    auto& allocator          = EventAllocators[writeIndex];

    void* eventDataMemory = allocator.allocate(eventType->Size, eventType->Alignment);
    if (eventDataMemory == nullptr) {
        assert(false && "Failed to allocate event data memory in ZEventBus::Publish");
        ZLOG_ERROR("EventBus") << "Failed to allocate event data memory for event: " << eventType->Name.c_str();
        return;
    }
    // Copy construct the event data
    if (eventType->CopyConstructor != nullptr) {
        eventType->CopyConstructor(eventDataMemory, eventData);
    }
    else {
        std::memcpy(eventDataMemory, eventData, eventType->Size);
    }

    EventsQueue.enqueue(EventPublishTask {.eventType = eventType, .eventData = eventDataMemory});
}

void ZEventBus::RegisterBaseEventsTypes()
{
    auto shutdownEngineEventType = CompileTimeTypeBuilder<ShutdownEngineEvent>::Build();
    TypeRegistry::RegisterType(shutdownEngineEventType, TypeKind::Struct);
}
void ZEventBus::UnregisterBaseEventsTypes() {
    TypeRegistry::UnregisterType<ShutdownEngineEvent>();
}

void ZEventBus::FlushEvents()
{
    ProcessSubscriptionCommands();
    const uint8_t readIndex  = ActiveWriteIndex.load(std::memory_order_relaxed);
    const uint8_t writeIndex = (readIndex + 1) % 2;
    ActiveWriteIndex.store(writeIndex, std::memory_order_relaxed);

    EventPublishTask buffer[kMaxBulkEventProcessCount];
    size_t count;
    while ((count = EventsQueue.try_dequeue_bulk(buffer, kMaxBulkEventProcessCount)) != 0) {
        for (size_t i = 0; i < count; ++i) {
            auto& curTask = buffer[i];

            if (!EventsCallbacks.contains(curTask.eventType)) {
                continue;
            }
            const DataStructures::ZBuffer<EventCallback>& eventCallbacks = EventsCallbacks[curTask.eventType];

            for (const auto& callback : eventCallbacks) {
                const bool consumed = callback(curTask.eventData);
                if (consumed) {
                    break;
                }
            }
        }
    }

    // Clear the read allocator
    EventAllocators[readIndex].clear();
}

void ZEventBus::ProcessSubscriptionCommands()
{
    SubscriptionCommand buffer[kMaxBulkEventProcessCount];
    size_t count;
    while ((count = SubscriptionQueue.try_dequeue_bulk(buffer, kMaxBulkEventProcessCount)) != 0) {
        for (size_t i = 0; i < count; ++i) {
            auto& cmd = buffer[i];
            if (cmd.action == SubscriptionAction::Subscribe) {
                auto it = EventsCallbacks.find(cmd.eventType);
                if (it == EventsCallbacks.end()) {
                    ZBuffer<EventCallback> callbacksBuffer;
                    EventsCallbacks[cmd.eventType] = std::move(callbacksBuffer);
                    it                             = EventsCallbacks.find(cmd.eventType);
                }
                it->second.push_back(std::move(cmd.callback));
                HandleToListenerMap[cmd.subscriptionHandle] = it->second.end() - 1;
            }
            else if (cmd.action == SubscriptionAction::Unsubscribe) {
                auto it = EventsCallbacks.find(cmd.eventType);
                if (it == EventsCallbacks.end()) {
                    continue;
                }
                auto* eventListener = HandleToListenerMap[cmd.subscriptionHandle];
                if (eventListener == nullptr) {
                    continue;
                }
                it->second.erase(eventListener);
            }
        }
    }
}