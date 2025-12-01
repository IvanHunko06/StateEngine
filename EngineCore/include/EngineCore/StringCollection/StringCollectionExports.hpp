#pragma once
#include "EngineCore/EngineCoreAPI.hpp"

extern "C" {
	ENGINE_CORE_API const char* StringCollection_GetOrCreateSharedString(const char* str);
	ENGINE_CORE_API void StringCollection_IncrementRefCount(const char* str);
	ENGINE_CORE_API void StringCollection_DecrementRefCount(const char* str);
}