#pragma once
#include "EngineCore/EngineCoreAPI.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
extern "C" {
ENGINE_CORE_API void ServiceLocator_RegisterService(const StateEngine::EngineCore::EngineTypeSystem::TypeInfo* type,
                                                    void* instance);
ENGINE_CORE_API void* ServiceLocator_GetService(const StateEngine::EngineCore::EngineTypeSystem::TypeInfo* type);
ENGINE_CORE_API void ServiceLocator_RemoveService(const StateEngine::EngineCore::EngineTypeSystem::TypeInfo* type);
}