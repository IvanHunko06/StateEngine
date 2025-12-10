#include "ZServiceLocator.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include <assert.h>

using namespace StateEngine::EngineCore::ServiceLocator;

ZHashMap<const TypeInfo*, void*> ZServiceLocator::registeredServices_;
bool ZServiceLocator::isSealed_;

void ZServiceLocator::RegisterService(const TypeInfo* type, void* instance) {
	assert(type && "type is null");
	assert(instance && "instance is null");
	if (!type || !instance) return;
	if (isSealed_) {
		assert(!isSealed_ && "Cannot register service after initialization!");
		return;
	}
	if (registeredServices_.contains(type)) {
		ZLOG_ERROR("EngineCore") << "Service '" << type->name.c_str() << "' is being registered twice!";
		assert(false && "Duplicate service registration!");
		return;
	}
	registeredServices_[type] = instance;
}
void* ZServiceLocator::GetService(const TypeInfo* type) {
	assert(type && "type is null");
	if (!type) return nullptr;

	return registeredServices_.contains(type) ? registeredServices_[type] : nullptr;
}
void ZServiceLocator::RemoveService(const TypeInfo* type) {
	assert(type && "type is null");
	if (!type) return;
	if (isSealed_) {
		assert(!isSealed_ && "Cannot remove service after initialization!");
		return;
	}
	registeredServices_.erase(type);
}