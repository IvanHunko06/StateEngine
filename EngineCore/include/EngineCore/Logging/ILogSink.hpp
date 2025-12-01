#pragma once
#include "EngineCore/Logging/LogLevel.hpp"

using StateEngine::EngineCore::Logging::LogLevel;
namespace StateEngine::EngineCore::Logging {
	class ILogSink {
	public:
		virtual void write(LogLevel level, const char* moduleName, const char* fileName, int line, const char* message) = 0;
	};

}