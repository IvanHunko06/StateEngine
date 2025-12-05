#include "EngineCore/Logging/LoggerExports.hpp"
#include "ZLogger.hpp"

using StateEngine::EngineCore::Logging::LogLevel;
using StateEngine::EngineCore::Logging::ZLogger;
extern "C" {
	ENGINE_CORE_API void Logger_Submit(LogLevel level, const ZString& sender, const ZString& fileName, int line, const ZString& message){
		ZLogger::Write(level, sender, fileName, line, message);
	}
	ENGINE_CORE_API void Logger_AddLogSink(ILogSink* sink) {
		ZLogger::AddLogSink(sink);
	}
	ENGINE_CORE_API bool Logger_IsLevelEnabled(LogLevel level, const ZString& moduleName) {
		return ZLogger::LogLevelEnabled(level, moduleName);
	}
	ENGINE_CORE_API LogLevel Logger_GetLogLevel(const ZString& moduleName) {
		return ZLogger::GetLogLevel(moduleName);
	}
	ENGINE_CORE_API void Logger_SetGlobalLogLevel(LogLevel level) {
		ZLogger::SetGlobalLogLevel(level);
	}
	ENGINE_CORE_API void Logger_OverrideLogLevel(const ZString& moduleName, LogLevel level) {
		ZLogger::OverrideLogLevel(moduleName, level);
	}
}