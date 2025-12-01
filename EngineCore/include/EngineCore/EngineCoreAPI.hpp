#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifdef ENGINE_CORE_EXPORTS
#define ENGINE_CORE_API __declspec(dllexport)
#else
#define ENGINE_CORE_API __declspec(dllimport)
#endif
#else
#define ENGINE_CORE_API __attribute__((visibility("default")))
#endif
