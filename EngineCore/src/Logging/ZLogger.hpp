#pragma once
#include "EngineCore/Logging/LogLevel.hpp"
#include "EngineCore/Logging/ILogSink.hpp"
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/DataStructures/ZString.hpp"
#include "EngineCore/Threading/MPMCQueue.hpp"
#include <mutex>

using namespace StateEngine::EngineCore::DataStructures;
using StateEngine::EngineCore::Threading::MPMCQueue;

namespace StateEngine::EngineCore::Logging {

	class ZLogger {
	public:
		static void Write(LogLevel level, const char* moduleName, const char* fileName, int line, const char* message);
		static void SetGlobalLogLevel(LogLevel level);
		static void OverrideLogLevel(const char* moduleName, LogLevel level);
		static void AddLogSink(ILogSink* sink);
		static bool LogLevelEnabled(LogLevel level, const char* moduleName);
		static LogLevel getLogLevel(const char* moduleName);
		static void FlushMessages();
	private:
		struct WriteMessageContext {
			LogLevel logLevel{};
			ZString moduleName{};
			ZString fileName{};
			int line{};
			ZString message{};
		};

		static bool changingSinksAndLevelsAllowed;
		static MPMCQueue<WriteMessageContext> messagesToLog_;
		static ZBuffer<ILogSink*> logSinks_;
		static ZHashMap<ZString, LogLevel> overrideLogLevels_;
		static LogLevel globalLogLevel_;
	};
}