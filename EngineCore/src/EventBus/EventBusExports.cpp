#include "EngineApplication.hpp"
#include "EngineCore/EngineCoreAPI.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/EventBus/IEventBus.hpp"
#include "ZEventBus.hpp"
#include <utility>

using namespace StateEngine::EngineCore::EventBus;
using namespace StateEngine::EngineCore::EngineTypeSystem;
using StateEngine::EngineCore::EngineApplication;

extern "C" {
ENGINE_CORE_API EventSubscriptionHandle GlobalEventBus_Subscribe(const TypeInfo* eventType, EventCallback&& callback)
{
    ZEventBus& eventBus = EngineApplication::GetGlobalEventBus();

    return eventBus.Subscribe(eventType, std::move(callback));
}
ENGINE_CORE_API void GlobalEventBus_Unsubscribe(const TypeInfo* eventType, EventSubscriptionHandle listenerHandle)
{
    ZEventBus& eventBus = EngineApplication::GetGlobalEventBus();

    eventBus.Unsubscribe(eventType, listenerHandle);
}
ENGINE_CORE_API void GlobalEventBus_Publish(const TypeInfo* eventType, const void* eventData)
{
    ZEventBus& eventBus = EngineApplication::GetGlobalEventBus();

    eventBus.Publish(eventType, eventData);
}
}