#pragma once
#include "InputEventsSystem.hpp"
#include "EngineCore/EventBus/EventBusWrapper.hpp"
#include "EngineCore/BaseEngineEvents/ShutdownEngineEvent.hpp"

using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::EngineCore::BaseEngineEvents;
using namespace StateEngine::EngineCore::EventBus;

void InputEventsSystem::UpdateSystem(float deltaTime) {
	SDL_Event event;

	while (gameWindowEventsQueue.pop(event)){
		if (event.type == SDL_EVENT_QUIT) {
            EventBusWrapper::Publish<ShutdownEngineEvent>(nullptr);
		}
	}
}