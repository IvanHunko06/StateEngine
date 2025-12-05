#pragma once
#include "EngineCore/Logging/LogLevel.hpp"

using StateEngine::EngineCore::Logging::LogLevel;
namespace StateEngine::EngineCore::Logging {
	class ILogSink {
	public:
		virtual void Write(LogLevel level, const char* sender, const char* fileName, int line, const char* message) = 0;
	};

}