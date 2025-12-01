#include "ZLogger.hpp"
#include <sstream>
using namespace StateEngine::EngineCore::Logging;

bool ZLogger::changingSinksAndLevelsAllowed = true;
MPMCQueue<ZLogger::WriteMessageContext> ZLogger::messagesToLog_;
ZBuffer<ILogSink*> ZLogger::logSinks_;
ZHashMap<ZString, LogLevel> ZLogger::overrideLogLevels_;
LogLevel ZLogger::globalLogLevel_ = LogLevel::INFO;

void ZLogger::Write(LogLevel level, const char* moduleName, const char* fileName, int line, const char* message) {
	if (!LogLevelEnabled(level, moduleName)) return;
	WriteMessageContext context{
		.logLevel = level,
		.moduleName = moduleName,
		.fileName = fileName,
		.line = line,
		.message = message
	};
	messagesToLog_.enqueue(context);
}
void ZLogger::SetGlobalLogLevel(LogLevel level) {
	if (!changingSinksAndLevelsAllowed) {
		assert(changingSinksAndLevelsAllowed && "You can set the global logging level only before Application_Run.");
		return;
	}
	globalLogLevel_ = level;
}
void ZLogger::OverrideLogLevel(const char* moduleName, LogLevel level) {
	if (!changingSinksAndLevelsAllowed) {
		assert(changingSinksAndLevelsAllowed && "You can override global logging level only before Application_Run.");
		return;
	}
	ZString moduleNameString = moduleName;
	if (!overrideLogLevels_.contains(moduleNameString)) {
		overrideLogLevels_[moduleNameString] = level;
		return;
	}
	overrideLogLevels_[moduleNameString] = level;
}
void ZLogger::AddLogSink(ILogSink* sink) {
	if (!sink) return;
	if (!changingSinksAndLevelsAllowed) {
		assert(changingSinksAndLevelsAllowed && "You can add log sink only before Application_Run.");
		return;
	}
	logSinks_.push_back(sink);
}

bool ZLogger::LogLevelEnabled(LogLevel level, const char* moduleName) {
	if (overrideLogLevels_.contains(moduleName))
		return level >= overrideLogLevels_[moduleName];
	else
		return level >= globalLogLevel_;
}
LogLevel ZLogger::getLogLevel(const char* moduleName) {
	if (overrideLogLevels_.contains(moduleName))
		return overrideLogLevels_[moduleName];
	else
		return globalLogLevel_;
}

void ZLogger::FlushMessages() {
	constexpr size_t kBulkSize = 32;
	WriteMessageContext buffer[kBulkSize];
	size_t count;

	while ((count = messagesToLog_.try_dequeue_bulk(buffer, kBulkSize)), count) {
		for (size_t i = 0; i < count; ++i) {
			auto& currentMessage = buffer[i];
			for (auto& sink : logSinks_) {
				sink->write(
					currentMessage.logLevel, 
					currentMessage.moduleName.c_str(), 
					currentMessage.fileName.c_str(), 
					currentMessage.line, 
					currentMessage.message.c_str()
				);
			}
		}
	}
}