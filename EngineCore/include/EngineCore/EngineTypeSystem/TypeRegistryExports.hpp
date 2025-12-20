#pragma once
#include "EngineCore/EngineCoreAPI.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"

using StateEngine::EngineCore::EngineTypeSystem::TypeInfo;
extern "C" {
ENGINE_CORE_API bool TypeRegistry_RegisterType(TypeInfo&& info);
ENGINE_CORE_API const TypeInfo& TypeRegistry_GetRequiredType(size_t hashCode);
ENGINE_CORE_API const TypeInfo* TypeRegistry_TryGetType(size_t hashCode);
ENGINE_CORE_API void TypeRegistry_RemoveType(size_t hashCode);
}