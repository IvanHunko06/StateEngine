#pragma once
#include "EngineCore/DataStructures/ZFunction.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
namespace StateEngine::EngineCore::EventBus {
    using EventSubscriptionHandle = size_t;

    /// <summary>
    /// Event listener callback function
    /// </summary>
    /// <param name="eventData">event data. may be null</param>
    /// <returns>Should the event be consumed? If true, subsequent callback functions of listeners for this event will
    /// not be called. If false, the remaining listeners will be processed.</returns>
    using EventCallback = DataStructures::ZFunction<bool(const void* eventData)>;
    class IEventBus {
        /// <summary>
        /// Subscribes to an event asynchronously. The actual subscription will only occur in the next frame.
        /// </summary>
        /// <param name="eventType">Event type metadata pointer</param>
        /// <param name="callback">The callback function of the listener for this event listener</param>
        /// <returns>This listener's unsubscribe handle</returns>
        virtual EventSubscriptionHandle Subscribe(const EngineTypeSystem::TypeInfo* eventType, EventCallback&& callback) = 0;
        /// <summary>
        /// Unsubscribe from an event asynchronously. The actual unsubscription will only occur on the next frame.
        /// </summary>
        /// <param name="eventType">Event type metadata pointer</param>
        /// <param name="listenerHandle">registered listener handle received upon subscription</param>
        virtual void Unsubscribe(const EngineTypeSystem::TypeInfo* eventType, EventSubscriptionHandle listenerHandle) = 0;
        /// <summary>
        /// Asynchronously publishes an event to the bus. Actual processing of all events and sending messages to all
        /// listeners will occur synchronously in the next frame on the main thread. Creates a local copy of the event
        /// data within the bus using the event type's constructor and destructor, which will be passed to listeners.
        /// </summary>
        /// <param name="eventType">Event type metadata pointer</param>
        /// <param name="eventData">Event data. Must be a type that describes the passed type.</param>
        virtual void Publish(const EngineTypeSystem::TypeInfo* eventType, const void* eventData) = 0;
    };
}  // namespace StateEngine::EngineCore::EventBus