#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifdef DESKTOP_PLATFORM_MODULE_EXPORTS
#define DESKTOP_PLATFORM_MODULE_API __declspec(dllexport)
#else
#define DESKTOP_PLATFORM_MODULE_API __declspec(dllimport)
#endif
#else
#define DESKTOP_PLATFORM_MODULE_API __attribute__((visibility("default")))
#endif
