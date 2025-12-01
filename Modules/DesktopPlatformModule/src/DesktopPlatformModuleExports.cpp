#include "DesktopPlatformModule/DesktopPlatformModuleExports.hpp"
#include "DesktopPlatformModule/DesktopPlatformModule.hpp"

using StateEngine::DesktopPlatformModule::DesktopPlatformModule;
extern "C" {
	DESKTOP_PLATFORM_MODULE_API IModule* CreateModule() {
		return &DesktopPlatformModule::getInstance();
	}
}