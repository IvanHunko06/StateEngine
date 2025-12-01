#include "ZServiceLocator.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"

using namespace StateEngine::EngineCore::ServiceLocator;

ZHashMap<const TypeInfo*, void*> ZServiceLocator::registeredServices_;
std::shared_mutex ZServiceLocator::servicesMutex_;

void ZServiceLocator::registerService(const TypeInfo* type, void* instance) {
	std::unique_lock<std::shared_mutex> lock(servicesMutex_);
	assert(type && "type is null");
	assert(instance && "instance is null");
	if (!type || !instance) return;

	if (registeredServices_.contains(type)) {
		ZLOG_ERROR("EngineCore") << "Service '" << type->name.c_str() << "' is being registered twice!";
		assert(false && "Duplicate service registration!");
		return;
	}
	registeredServices_[type] = instance;
}
void* ZServiceLocator::getService(const TypeInfo* type) {
	assert(type && "type is null");
	if (!type) return nullptr;
	std::unique_lock<std::shared_mutex> lock(servicesMutex_);

	return registeredServices_.contains(type) ? registeredServices_[type] : nullptr;
}
void  ZServiceLocator::removeService(const TypeInfo* type) {
	assert(type && "type is null");
	if (!type) return;
	std::unique_lock<std::shared_mutex> lock(servicesMutex_);

	registeredServices_.erase(type);
}