#include "ZTypeRegistry.hpp"
#include "EngineCore/EngineTypeSystem/ReflectionMacros.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include <cassert>
using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::EngineCore::DataStructures;

ZHashMap<size_t, TypeInfo> ZTypeRegistry::mapTypes_;
bool ZTypeRegistry::recordingAllowed_ = true;

bool ZTypeRegistry::RegisterType(TypeInfo&& type) {
	if (!recordingAllowed_) {
		assert(recordingAllowed_ && "Addition is only allowed in the RegisterTypes function.");
		return false;
	}
	if (mapTypes_.contains(type.hashCode)) {
		TypeInfo& registredType = mapTypes_[type.hashCode];
		ZLOG_WARN("EngineCore") 
			<< "A hash collision was detected. The new type's " << type.name.c_str() 
			<< "hash matches an already registered type " << registredType.name.c_str() 
			<< ". The new type will not be added.";
		return false;
	}

	mapTypes_[type.hashCode] = std::move(type);
	return true;
}
const TypeInfo* ZTypeRegistry::GetTypeInfo(size_t hashCode) {	
	return mapTypes_.contains(hashCode) ? &mapTypes_[hashCode] : nullptr;
}

void ZTypeRegistry::RemoveType(size_t hashCode) {
	if (!recordingAllowed_) {
		assert(recordingAllowed_ && "Deletion is only allowed in the UnregisterTypes function.");
		return;
	}
	
	mapTypes_.erase(hashCode);
}

void ZTypeRegistry::RegisterBaseTypes() {
	REGISTER_PRIMITIVE("void", void);
	REGISTER_PRIMITIVE("bool", bool);

	REGISTER_PRIMITIVE("int8", int8_t);
	REGISTER_PRIMITIVE("uint8", uint8_t);

	REGISTER_PRIMITIVE("int16", int16_t);
	REGISTER_PRIMITIVE("uint16", uint16_t);

	REGISTER_PRIMITIVE("int32", int32_t);
	REGISTER_PRIMITIVE("uint32", uint32_t);

	REGISTER_PRIMITIVE("int64", int64_t);
	REGISTER_PRIMITIVE("uint64", uint64_t);

}