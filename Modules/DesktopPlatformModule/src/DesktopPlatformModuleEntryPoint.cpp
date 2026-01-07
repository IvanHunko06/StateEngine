#include "DesktopPlatformModule/DesktopPlatformModule.hpp"
#include "DesktopPlatformModule/DesktopPlatformModuleAPI.hpp"
#include "EngineCore/IModule.hpp"

using StateEngine::DesktopPlatformModule::DesktopPlatformModule;
using StateEngine::EngineCore::IModule;
extern "C" {
DESKTOP_PLATFORM_MODULE_API IModule* CreateModule()
{
    return &DesktopPlatformModule::GetInstance();
}
}

#ifdef _WIN32
#include <windows.h>
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}
#endif
