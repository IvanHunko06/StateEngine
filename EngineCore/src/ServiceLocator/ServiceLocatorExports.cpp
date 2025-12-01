#include "EngineCore/ServiceLocator/ServiceLocatorExports.hpp"
#include "ZServiceLocator.hpp"

using StateEngine::EngineCore::EngineTypeSystem::TypeInfo;
using StateEngine::EngineCore::ServiceLocator::ZServiceLocator;

extern "C" {
	ENGINE_CORE_API void ServiceLocator_RegisterService(const TypeInfo* type, void* instance) {
		ZServiceLocator::registerService(type, instance);
	}
	ENGINE_CORE_API void* ServiceLocator_GetService(const TypeInfo* type) {
		return ZServiceLocator::getService(type);
	}
	ENGINE_CORE_API void ServiceLocator_RemoveService(const TypeInfo* type) {
		ZServiceLocator::removeService(type);
	}
}