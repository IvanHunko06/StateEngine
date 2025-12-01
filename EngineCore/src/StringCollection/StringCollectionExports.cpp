#include "EngineCore/StringCollection/StringCollectionExports.hpp"
#include "ZStringCollection.hpp"

using namespace StateEngine::EngineCore::StringCollection;

extern "C" {
	ENGINE_CORE_API const char* StringCollection_GetOrCreateSharedString(const char* str) {
		return ZStringCollection::getOrCreateSharedString(str);
	}
	ENGINE_CORE_API void StringCollection_IncrementRefCount(const char* str) {
		ZStringCollection::incrementRefCount(str);
	}
	ENGINE_CORE_API void StringCollection_DecrementRefCount(const char* str) {
		ZStringCollection::decrementRefCount(str);
	}
}