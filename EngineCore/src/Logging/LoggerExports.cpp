#include "EngineCore/Logging/LoggerExports.hpp"
#include "ZLogger.hpp"

using StateEngine::EngineCore::Logging::LogLevel;
using StateEngine::EngineCore::Logging::ZLogger;
extern "C" {
	ENGINE_CORE_API void Logger_Submit(LogLevel level, const char* moduleName, const char* fileName, int line, const char* message){
		ZLogger::Write(level, moduleName, fileName, line, message);
	}
	ENGINE_CORE_API void Logger_AddLogSink(ILogSink* sink) {
		ZLogger::AddLogSink(sink);
	}

	ENGINE_CORE_API bool Logger_IsLevelEnabled(LogLevel level, const char* moduleName) {
		return ZLogger::LogLevelEnabled(level, moduleName);
	}
	ENGINE_CORE_API LogLevel Logger_GetLogLevel(const char* moduleName) {
		return ZLogger::getLogLevel(moduleName);
	}
	ENGINE_CORE_API void Logger_SetGlobalLogLevel(LogLevel level) {
		ZLogger::SetGlobalLogLevel(level);
	}
	ENGINE_CORE_API void Logger_OverrideLogLevel(const char* moduleName, LogLevel level) {
		ZLogger::OverrideLogLevel(moduleName, level);
	}
}