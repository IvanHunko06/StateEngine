#pragma once
#include <cstdint>
struct VulkanCreateInstanceConfiguration {
	struct VulkanVersion {
		uint8_t major{ 1 };
		uint8_t minor{ 0 };
		uint8_t patch{ 0 };
	};
	const char* applicationName{ "No ApplicationName"};
	VulkanVersion applicationVersion{};
	const char* engineName{ "No EngineName" };
	VulkanVersion engineVersion{};
};