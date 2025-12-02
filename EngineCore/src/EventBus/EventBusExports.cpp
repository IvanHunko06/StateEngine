#include "EngineCore/EventBus/EventBusExports.hpp"
#include "ZEventBus.hpp"

using namespace StateEngine::EngineCore::EventBus;

extern "C" {
	ENGINE_CORE_API void EventBus_Subscribe(const TypeInfo* eventType, void* listener, EventCallback callback) {
		ZEventBus::Subscribe(eventType, listener, callback);
	}
	ENGINE_CORE_API void EventBus_Unsubscribe(const TypeInfo* eventType, void* listener, EventCallback callback) {
		ZEventBus::Unsubscribe(eventType, listener, callback);
	}
	ENGINE_CORE_API void EventBus_Publish(const TypeInfo* eventType, void* userdata) {
		ZEventBus::Publish(eventType, userdata);
	}
}