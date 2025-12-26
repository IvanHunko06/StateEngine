#include "EngineCore/StringCollection/StringCollectionExports.hpp"
#include "ZStringCollection.hpp"

using namespace StateEngine::EngineCore::StringCollection;

extern "C" {
	ENGINE_CORE_API const char* StringCollection_GetOrCreateSharedString(const char* str) {
		return ZStringCollection::GetOrCreateSharedString(str);
	}
	ENGINE_CORE_API void StringCollection_IncrementRefCount(const char* str) {
		ZStringCollection::IncrementRefCount(str);
	}
	ENGINE_CORE_API void StringCollection_DecrementRefCount(const char* str) {
		ZStringCollection::DecrementRefCount(str);
	}
}