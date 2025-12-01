#pragma once
#include "EngineCore/EngineCoreAPI.hpp"
#include "EngineCore/Logging/LogLevel.hpp"
#include "EngineCore/Logging/ILogSink.hpp"

using StateEngine::EngineCore::Logging::LogLevel;
using StateEngine::EngineCore::Logging::ILogSink;

extern "C" {
	ENGINE_CORE_API void Logger_Submit(LogLevel level, const char* moduleName, const char* fileName, int line, const char* message);
	ENGINE_CORE_API void Logger_AddLogSink(ILogSink* sink);

	ENGINE_CORE_API bool Logger_IsLevelEnabled(LogLevel level, const char* moduleName);
	ENGINE_CORE_API LogLevel Logger_GetLogLevel(const char* moduleName);
	ENGINE_CORE_API void Logger_SetGlobalLogLevel(LogLevel level);
	ENGINE_CORE_API void Logger_OverrideLogLevel(const char* moduleName, LogLevel level);
}