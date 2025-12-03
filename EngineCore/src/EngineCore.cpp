#include "MemoryManagment/ZAllocator_SEMalloc_Virtual.hpp"
#include "StringCollection/ZStringCollection.hpp"
#include "EngineTypeSystem/ZTypeRegistry.hpp"
#include "EventBus/ZEventBus.hpp"
#ifdef USE_MIMALLOC_ALLOCATOR
#include "mimalloc.h"
#endif
using StateEngine::EngineCore::StringCollection::ZStringCollection;
using StateEngine::EngineCore::EngineTypeSystem::ZTypeRegistry;
using StateEngine::EngineCore::EventBus::ZEventBus;

#ifdef _WIN32
#include <windows.h>
#ifdef USE_MIMALLOC_ALLOCATOR
void MimallocOutput(const char* msg, void* arg) {
	OutputDebugStringA(msg);
}
#endif
BOOL APIENTRY DllMain(HMODULE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved
					 )
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
#ifdef USE_MIMALLOC_ALLOCATOR
		mi_register_output(MimallocOutput, nullptr);
		mi_option_enable(mi_option_show_stats);
#endif
		ZStringCollection::init();
		ZTypeRegistry::RegisterBaseTypes();
		ZEventBus::RegisterBaseEvents();
	}
	return TRUE;
}
#endif
