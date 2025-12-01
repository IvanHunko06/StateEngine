#pragma once
#include "EngineCore/EngineCoreAPI.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
using StateEngine::EngineCore::EngineTypeSystem::TypeInfo;

extern "C" {
	ENGINE_CORE_API void ServiceLocator_RegisterService(const TypeInfo* type, void* instance);
	ENGINE_CORE_API void* ServiceLocator_GetService(const TypeInfo* type);
	ENGINE_CORE_API void ServiceLocator_RemoveService(const TypeInfo* type);
}