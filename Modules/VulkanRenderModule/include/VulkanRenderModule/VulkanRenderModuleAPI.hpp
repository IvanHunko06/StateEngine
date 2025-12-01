#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifdef VULKAN_RENDER_MODULE_EXPORTS
#define VULKAN_RENDER_MODULE_API __declspec(dllexport)
#else
#define VULKAN_RENDER_MODULE_API __declspec(dllimport)
#endif
#else
#define VULKAN_RENDER_MODULE_API __attribute__((visibility("default")))
#endif