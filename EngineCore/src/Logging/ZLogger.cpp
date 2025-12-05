#include "ZLogger.hpp"
#include <sstream>
using namespace StateEngine::EngineCore::Logging;

bool ZLogger::sinksAndLevelsSealed_;
MPMCQueue<ZLogger::WriteMessageCommand> ZLogger::messagesToLog_;
ZBuffer<ILogSink*> ZLogger::logSinks_;
ZHashMap<ZString, LogLevel> ZLogger::overrideLogLevels_;
LogLevel ZLogger::globalLogLevel_ = LogLevel::INFO;

void ZLogger::Write(LogLevel level, const ZString& sender, const ZString& fileName, int line, const ZString& message) {
	if (!LogLevelEnabled(level, sender)) return;
	WriteMessageCommand context{
		.logLevel = level,
		.sender = sender,
		.fileName = fileName,
		.line = line,
		.message = message
	};
	messagesToLog_.enqueue(context);
}
void ZLogger::SetGlobalLogLevel(LogLevel level) {
	if (sinksAndLevelsSealed_) {
		assert(!sinksAndLevelsSealed_ && "You can set the global logging level only before Application_Run.");
		return;
	}
	globalLogLevel_ = level;
}
void ZLogger::OverrideLogLevel(const ZString& sender, LogLevel level) {
	if (sinksAndLevelsSealed_) {
		assert(!sinksAndLevelsSealed_ && "You can override logging level only before Application_Run.");
		return;
	}
	if (!overrideLogLevels_.contains(sender)) {
		overrideLogLevels_[sender] = level;
		return;
	}
	overrideLogLevels_[sender] = level;
}
void ZLogger::AddLogSink(ILogSink* sink) {
	if (!sink) return;
	if (sinksAndLevelsSealed_) {
		assert(!sinksAndLevelsSealed_ && "You can add log sink only before Application_Run.");
		return;
	}
	logSinks_.push_back(sink);
}

bool ZLogger::LogLevelEnabled(LogLevel level, const ZString& sender) {
	if (overrideLogLevels_.contains(sender))
		return level >= overrideLogLevels_[sender];
	else
		return level >= globalLogLevel_;
}
LogLevel ZLogger::GetLogLevel(const ZString& sender) {
	if (overrideLogLevels_.contains(sender))
		return overrideLogLevels_[sender];
	else
		return globalLogLevel_;
}

void ZLogger::FlushMessages() {
	constexpr size_t kBulkSize = 32;
	WriteMessageCommand buffer[kBulkSize];
	size_t count;

	while ((count = messagesToLog_.try_dequeue_bulk(buffer, kBulkSize)), count) {
		for (size_t i = 0; i < count; ++i) {
			auto& currentMessage = buffer[i];
			for (auto& sink : logSinks_) {
				sink->Write(
					currentMessage.logLevel, 
					currentMessage.sender.c_str(), 
					currentMessage.fileName.c_str(), 
					currentMessage.line, 
					currentMessage.message.c_str()
				);
			}
		}
	}
}