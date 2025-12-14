#pragma once
#include "EngineCore/IUpdatableSystem.hpp"
#include "EngineCore/Threading/SPSCQueue.hpp"
#include "NativeWindow.hpp"
using StateEngine::EngineCore::IEngineSystem;

class InputEventsSystem : public IEngineSystem{
public:
	NativeWindow::WindowEventsQueue gameWindowEventsQueue;
	void UpdateSystem(float deltaTime);
};