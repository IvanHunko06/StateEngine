#pragma once
#include "EngineCoreAPI.hpp"
#include "ApplicationConfigurations/IApplication.hpp"
#include "EngineCore/EngineUpdatePhase.hpp"
#include "EngineCore/IUpdatableSystem.hpp"
using StateEngine::EngineCore::ApplicationConfigurations::IApplication;
using StateEngine::EngineCore::EngineUpdatePhase;
using StateEngine::EngineCore::IEngineSystem;

extern "C" {
	ENGINE_CORE_API void Application_Run(IApplication* app);
	ENGINE_CORE_API void Application_RegisterSystem(EngineUpdatePhase phase, IEngineSystem* system);
}