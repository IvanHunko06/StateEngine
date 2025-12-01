#pragma once
#ifdef ERROR
#undef ERROR
#endif // ERROR
namespace StateEngine::EngineCore::Logging {
	enum class LogLevel {
		DEBUG,
		INFO,
		WARNING,
		ERROR,
		FATAL
	};
}