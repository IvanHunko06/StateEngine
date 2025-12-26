#pragma once
#include "EngineCore/EngineCoreAPI.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"

extern "C" {
ENGINE_CORE_API bool TypeRegistry_RegisterType(StateEngine::EngineCore::EngineTypeSystem::TypeInfo&& info);
ENGINE_CORE_API const StateEngine::EngineCore::EngineTypeSystem::TypeInfo* TypeRegistry_GetType(StateEngine::EngineCore::EngineTypeSystem::TypeKey hashCode);
ENGINE_CORE_API void TypeRegistry_RemoveType(StateEngine::EngineCore::EngineTypeSystem::TypeKey hashCode);
}