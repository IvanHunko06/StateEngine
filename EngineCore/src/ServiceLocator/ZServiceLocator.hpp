#pragma once
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"

using StateEngine::EngineCore::DataStructures::ZHashMap;
using StateEngine::EngineCore::EngineTypeSystem::TypeInfo;

namespace StateEngine::EngineCore::ServiceLocator {
	class ZServiceLocator {
	private:
		static ZHashMap<const TypeInfo*, void*> RegisteredServices;
		static bool IsSealed;
	public:
		static void  RegisterService(const TypeInfo* type, void* instance);
		static void* GetService(const TypeInfo* type);
		static void  RemoveService(const TypeInfo* type);
		static inline void SetIsSealed(bool sealed) noexcept {
			IsSealed = sealed;
		}
	};
}