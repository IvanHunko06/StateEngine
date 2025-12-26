#pragma once
#include "EngineCore/EngineCoreAPI.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/EventBus/IEventBus.hpp"
extern "C" {
ENGINE_CORE_API StateEngine::EngineCore::EventBus::EventSubscriptionHandle
GlobalEventBus_Subscribe(const StateEngine::EngineCore::EngineTypeSystem::TypeInfo* eventType,
                         StateEngine::EngineCore::EventBus::EventCallback&& callback);
ENGINE_CORE_API void
GlobalEventBus_Unsubscribe(const StateEngine::EngineCore::EngineTypeSystem::TypeInfo* eventType,
                           StateEngine::EngineCore::EventBus::EventSubscriptionHandle listenerHandle);
ENGINE_CORE_API void GlobalEventBus_Publish(const StateEngine::EngineCore::EngineTypeSystem::TypeInfo* eventType,
                                            const void* eventData);
}