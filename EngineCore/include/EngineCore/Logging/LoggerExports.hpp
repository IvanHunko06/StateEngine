#pragma once
#include "EngineCore/EngineCoreAPI.hpp"
#include "EngineCore/Logging/LogLevel.hpp"
#include "EngineCore/Logging/ILogSink.hpp"
#include "EngineCore/DataStructures/ZString.hpp"

using StateEngine::EngineCore::Logging::LogLevel;
using StateEngine::EngineCore::Logging::ILogSink;
using StateEngine::EngineCore::DataStructures::ZString;

extern "C" {
	ENGINE_CORE_API void Logger_Submit(LogLevel level, const ZString& sender, const ZString& fileName, int line, const ZString& message);
	ENGINE_CORE_API void Logger_AddLogSink(ILogSink* sink);
	ENGINE_CORE_API bool Logger_IsLevelEnabled(LogLevel level, const ZString& sender);
	ENGINE_CORE_API LogLevel Logger_GetLogLevel(const ZString& sender);
	ENGINE_CORE_API void Logger_SetGlobalLogLevel(LogLevel level);
	ENGINE_CORE_API void Logger_OverrideLogLevel(const ZString& sender, LogLevel level);
}