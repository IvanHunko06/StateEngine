#pragma once
#include "ZLogStream.hpp"
#include "LoggerExports.hpp"

#define ZLOG_STREAM(level, moduleName) \
    if (Logger_IsLevelEnabled(level, moduleName)) \
         StateEngine::EngineCore::Logging::ZLogStream(level, moduleName)

#define ZLOG_DEBUG(moduleName)   ZLOG_STREAM(StateEngine::EngineCore::Logging::LogLevel::DEBUG, moduleName)
#define ZLOG_INFO(moduleName)    ZLOG_STREAM(StateEngine::EngineCore::Logging::LogLevel::INFO,  moduleName)
#define ZLOG_WARN(moduleName)    ZLOG_STREAM(StateEngine::EngineCore::Logging::LogLevel::WARNING,  moduleName)
#define ZLOG_ERROR(moduleName)   ZLOG_STREAM(StateEngine::EngineCore::Logging::LogLevel::ERROR, moduleName)
#define ZLOG_FATAL(moduleName)   ZLOG_STREAM(StateEngine::EngineCore::Logging::LogLevel::FATAL, moduleName)