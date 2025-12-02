#ifdef _WIN32
#include <windows.h>
#include "MemoryManagment/ZAllocator_SEMalloc_Virtual.hpp"
#include "StringCollection/ZStringCollection.hpp"
#include "EngineTypeSystem/ZTypeRegistry.hpp"
#include "EventBus/ZEventBus.hpp"
//using StateEngine::EngineCore::MemoryManagment::ZAllocator_SEMalloc_Virtual;
using StateEngine::EngineCore::StringCollection::ZStringCollection;
using StateEngine::EngineCore::EngineTypeSystem::ZTypeRegistry;
using StateEngine::EngineCore::EventBus::ZEventBus;
BOOL APIENTRY DllMain(HMODULE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved
					 )
{
	if (ul_reason_for_call == DLL_PROCESS_DETACH) {
		//ZStringCollection::release();
		//ZAllocator_SEMalloc_Virtual::releaseAllArenas();
	}
	else if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		ZStringCollection::init();
		ZTypeRegistry::RegisterBaseTypes();
		ZEventBus::RegisterBaseEvents();
	}
	return TRUE;
}
#endif
