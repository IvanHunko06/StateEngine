#pragma once
#include <cstdint>
namespace StateEngine::EngineCore::EngineTypeSystem {
	struct EnumMember {
		uint32_t numberValue{ 0 };
		const char* stringValue{ nullptr };
		EnumMember(const char* strVal, uint32_t numVal)
			: numberValue(numVal), stringValue(strVal) {}
		EnumMember() = default;


	};
}