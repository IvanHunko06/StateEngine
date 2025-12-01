#pragma once
#include <sstream>
#include <source_location>
#include "EngineCore/Logging/LogLevel.hpp"
#include "EngineCore/Logging/LoggerExports.hpp"

namespace StateEngine::EngineCore::Logging {
	class ZLogStream {
	private:
		std::stringstream buffer_;
		LogLevel logLevel_;
		const char* moduleName_;
		const char* fileName_;
		int line_;
	private:
		constexpr const char* GetFileNameFromPath(const char* path) const noexcept {
			const char* fileName = path;
			while (*path != '\0'){
				if (*path == '/' || *path == '\\'){
					fileName = path + 1;
				}
				path++;
			}
			return fileName;
		}
	public:
		 ZLogStream(LogLevel level, const char* moduleName, std::source_location loc = std::source_location::current()) noexcept
			: logLevel_(level), moduleName_(moduleName),
			 fileName_(GetFileNameFromPath(loc.file_name())),
			 line_(loc.line())
		
		{
		}
		~ZLogStream() noexcept{
			std::string formattedMessage = buffer_.str();
			if (formattedMessage.empty()) return;
			Logger_Submit(logLevel_, moduleName_, fileName_, line_, formattedMessage.c_str());
		}

		template<typename T>
		ZLogStream& operator<<(const T& data) {
			buffer_ << data;
			return *this;
		}

		ZLogStream(const ZLogStream&) = delete;
		ZLogStream& operator=(const ZLogStream&) = delete;

		ZLogStream(ZLogStream&& other) noexcept
			:	buffer_(std::move(other.buffer_)), logLevel_(other.logLevel_), 
				moduleName_(other.moduleName_), fileName_(other.fileName_), 
				line_(other.line_)
		{

		}
		ZLogStream& operator=(ZLogStream&& other) noexcept{
			if (this != &other) {
				buffer_ = std::move(other.buffer_);
				logLevel_ = other.logLevel_;
				moduleName_ = other.moduleName_;
				fileName_ = other.fileName_;
				line_ = other.line_;
			}
		}

	};
}