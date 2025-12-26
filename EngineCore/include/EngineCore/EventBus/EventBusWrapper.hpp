#pragma once
#include "EngineCore/DataStructures/ZFunction.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistry.hpp"
#include "EventBusExports.hpp"
#include "IEventBus.hpp"
#include <type_traits>

namespace StateEngine::EngineCore::EventBus {
    template <typename T>
    concept EventBusRestrictionConcept =
        requires { EngineTypeSystem::TypeRegistryRestrictionConcept<T>&& std::is_class_v<T>; };

    class EventBusWrapper {
      public:
        template <typename T>
            requires EventBusRestrictionConcept<T>
        using ParameterizedEventCallback = DataStructures::ZFunction<bool(const T* eventData)>;

        template <typename T>
            requires EventBusRestrictionConcept<T>
        static inline EventBus::EventSubscriptionHandle Subscribe(ParameterizedEventCallback<T>&& callback)
        {
            auto& type = EngineTypeSystem::TypeRegistry::GetRequiredType<T>();
            EventCallback typeErasedCallback = [cb = std::move(callback)](const void* eventData) -> bool{
                return cb(static_cast<const T*>(eventData));
            };
            return GlobalEventBus_Subscribe(&type, std::move(typeErasedCallback));
        }
        template <typename T>
            requires EventBusRestrictionConcept<T>
        static inline EventBus::EventSubscriptionHandle Subscribe(IEventBus* eventBus, ParameterizedEventCallback<T>&& callback)
        {
            auto& type = EngineTypeSystem::TypeRegistry::GetRequiredType<T>();
            EventCallback typeErasedCallback = [cb = std::move(callback)](const void* eventData) -> bool {
                return cb(static_cast<const T*>(eventData));
            };
            return eventBus->Subscribe(&type, std::move(typeErasedCallback));
        }

        template <typename T>
            requires EventBusRestrictionConcept<T>
        static inline void Unsubscribe(EventBus::EventSubscriptionHandle listenerHandle)
        {
            auto& type = EngineTypeSystem::TypeRegistry::GetRequiredType<T>();
            GlobalEventBus_Unsubscribe(&type, listenerHandle);
        }
        template <typename T>
            requires EventBusRestrictionConcept<T>
        static inline void Unsubscribe(IEventBus* eventBus, EventBus::EventSubscriptionHandle listenerHandle)
        {
            auto& type = EngineTypeSystem::TypeRegistry::GetRequiredType<T>();
            eventBus->Unsubscribe(&type, listenerHandle);
        }

        template <typename T>
            requires EventBusRestrictionConcept<T>
        static inline void Publish(const T* eventData)
        {
            auto& type = EngineTypeSystem::TypeRegistry::GetRequiredType<T>();
            GlobalEventBus_Publish(&type, eventData);
        }
        template <typename T>
            requires EventBusRestrictionConcept<T>
        static inline void Publish(IEventBus* eventBus, const T* eventData)
        {
            auto& type = EngineTypeSystem::TypeRegistry::GetRequiredType<T>();
            eventBus->Publish(&type, eventData);
        }
    };
}  // namespace StateEngine::EngineCore::EventBus