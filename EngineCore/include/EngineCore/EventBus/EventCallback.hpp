#pragma once
namespace StateEngine::EngineCore::EventBus {
	using EventCallback = bool(*)(void* listener, void* userData);
}