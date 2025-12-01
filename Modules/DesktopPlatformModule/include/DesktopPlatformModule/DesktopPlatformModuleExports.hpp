#pragma once
#include "DesktopPlatformModuleAPI.hpp"
#include "EngineCore/IModule.hpp"

using StateEngine::EngineCore::IModule;

extern "C" {
	DESKTOP_PLATFORM_MODULE_API IModule* CreateModule();
}