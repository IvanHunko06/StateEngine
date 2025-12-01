#pragma once
#include "EngineCore/EngineCoreAPI.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EventCallback.hpp"

using StateEngine::EngineCore::EventBus::EventCallback;
using StateEngine::EngineCore::EngineTypeSystem::TypeInfo;
extern "C" {
	ENGINE_CORE_API void EventBus_Subscribe(const TypeInfo* eventType, void* listener, EventCallback callback);
	ENGINE_CORE_API void EventBus_Unsubscribe(const TypeInfo* eventType, void* listener, EventCallback callback);
	ENGINE_CORE_API void EventBus_Publish(const TypeInfo* eventType, void* userdata);
}