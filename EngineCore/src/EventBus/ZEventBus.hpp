#pragma once
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/EventBus/IEventBus.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/MemoryManagment/ZFrameAllocator.hpp"
#include "EngineCore/SmartPointers/ZUniquePointer.hpp"
#include "EngineCore/Threading/MPMCQueue.hpp"

namespace StateEngine::EngineCore::EventBus {
    class ZEventBus : public IEventBus {
      private:
        enum class SubscriptionAction : uint8_t { Subscribe, Unsubscribe };
        struct SubscriptionCommand {
            SubscriptionAction action;
            const EngineTypeSystem::TypeInfo* eventType;
            EventCallback callback;
            EventSubscriptionHandle subscriptionHandle;
        };
        struct EventPublishTask {
            const EngineTypeSystem::TypeInfo* eventType {nullptr};
            void* eventData {nullptr};
        };

      private:
        DataStructures::ZHashMap<const EngineTypeSystem::TypeInfo*, DataStructures::ZBuffer<EventCallback>>
            EventsCallbacks;
        Threading::MPMCQueue<EventPublishTask> EventsQueue;
        Threading::MPMCQueue<SubscriptionCommand> SubscriptionQueue;
        static constexpr size_t kMaxBulkEventProcessCount = 128;
        std::array<MemoryManagment::ZFrameAllocator, 2> EventAllocators;
        std::atomic<uint8_t> ActiveWriteIndex;
        std::atomic<EventSubscriptionHandle> CurrentEventSubscriptionHandle {1};
        DataStructures::ZHashMap<EventSubscriptionHandle, EventCallback*> HandleToListenerMap;

      public:
        ZEventBus(size_t fixedAllocatorSize = 2 * 1024 * 1024) :
            EventAllocators {MemoryManagment::ZFrameAllocator(fixedAllocatorSize),
                             MemoryManagment::ZFrameAllocator(fixedAllocatorSize)}
        {
        }
        EventSubscriptionHandle Subscribe(const EngineTypeSystem::TypeInfo* eventType, EventCallback&& callback);
        void Unsubscribe(const EngineTypeSystem::TypeInfo* eventType, EventSubscriptionHandle listenerHandle);
        void Publish(const EngineTypeSystem::TypeInfo* eventType, const void* eventData);
        void FlushEvents();
        static void RegisterBaseEventsTypes();
        static void UnregisterBaseEventsTypes();

      private:
        void ProcessSubscriptionCommands();
    };
}  // namespace StateEngine::EngineCore::EventBus