#pragma once
#include "ApplicationConfigurations/IApplication.hpp"
#include "EngineCore/EngineUpdatePhase.hpp"
#include "EngineCore/IEngineSystem.hpp"
#include "EngineCoreAPI.hpp"

extern "C" {
ENGINE_CORE_API void Application_Run(StateEngine::EngineCore::ApplicationConfigurations::IApplication* app);
ENGINE_CORE_API void Application_RegisterSystem(StateEngine::EngineCore::EngineUpdatePhase phase,
                                                StateEngine::EngineCore::IEngineSystem* system);
}