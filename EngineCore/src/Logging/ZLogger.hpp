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
		static void Write(LogLevel level, const ZString& sender, const ZString& fileName, int line, const ZString& message);
		static void SetGlobalLogLevel(LogLevel level);
		static void OverrideLogLevel(const ZString& sender, LogLevel level);
		static void AddLogSink(ILogSink* sink);
		static bool LogLevelEnabled(LogLevel level, const ZString& sender);
		static LogLevel GetLogLevel(const ZString& sender);
		static void FlushMessages();
		static inline void SetSinksAndLevelsSealed(bool sealed) { sinksAndLevelsSealed_ = sealed; }
	private:
		struct WriteMessageCommand {
			LogLevel logLevel{};
			ZString sender{};
			ZString fileName{};
			int line{};
			ZString message{};
		};
		static bool sinksAndLevelsSealed_;
		static MPMCQueue<WriteMessageCommand> messagesToLog_;
		static ZBuffer<ILogSink*> logSinks_;
		static ZHashMap<ZString, LogLevel> overrideLogLevels_;
		static LogLevel globalLogLevel_;
	};
}