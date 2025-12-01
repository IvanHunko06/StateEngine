#pragma once
#include "EngineCore/IUpdatableSystem.hpp"
#include "EngineCore/Threading/SPSCQueue.hpp"
#include "NativeWindow.hpp"
using StateEngine::EngineCore::IUpdatableSystem;

class InputEventsSystem : public IUpdatableSystem{
public:
	NativeWindow::WindowEventsQueue gameWindowEventsQueue;
	void UpdateSystem(float deltaTime);
};