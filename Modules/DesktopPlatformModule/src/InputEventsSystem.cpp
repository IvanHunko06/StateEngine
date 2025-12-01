#pragma once
#include "InputEventsSystem.hpp"
#include "EngineCore/EventBus/EventBusExports.hpp"
#include "EngineCore/BaseEngineEvents/ShutdownEngineEvent.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistryMacros.hpp"

using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::EngineCore::BaseEngineEvents;

void InputEventsSystem::UpdateSystem(float deltaTime) {
	SDL_Event event;

	while (gameWindowEventsQueue.pop(event)){
		if (event.type == SDL_EVENT_QUIT) {
			EventBus_Publish(
				GET_TYPE_INFO("ShutdownEngineEvent"),
				nullptr
			);
		}
	}
}