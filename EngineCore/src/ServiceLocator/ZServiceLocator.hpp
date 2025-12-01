#pragma once
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include <shared_mutex>

using StateEngine::EngineCore::DataStructures::ZHashMap;
using StateEngine::EngineCore::EngineTypeSystem::TypeInfo;

namespace StateEngine::EngineCore::ServiceLocator {
	class ZServiceLocator {
	private:
		static ZHashMap<const TypeInfo*, void*> registeredServices_;
		static std::shared_mutex servicesMutex_;
	public:
		static void  registerService(const TypeInfo* type, void* instance);
		static void* getService(const TypeInfo* type);
		static void  removeService(const TypeInfo* type);
	};
}