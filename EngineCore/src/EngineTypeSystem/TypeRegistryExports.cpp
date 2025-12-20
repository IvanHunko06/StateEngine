#include "EngineCore/EngineTypeSystem/TypeRegistryExports.hpp"
#include "EngineTypeSystem/ZTypeRegistry.hpp"

using StateEngine::EngineCore::EngineTypeSystem::ZTypeRegistry;
extern "C" {
	ENGINE_CORE_API bool TypeRegistry_RegisterType(TypeInfo&& info) {
		return ZTypeRegistry::RegisterType(std::move(info));
	}
	ENGINE_CORE_API void TypeRegistry_RemoveType(size_t hashCode) {
		ZTypeRegistry::RemoveType(hashCode);
	}
	ENGINE_CORE_API const TypeInfo& TypeRegistry_GetRequiredType(size_t hashCode) {
        return ZTypeRegistry::GetRequiredTypeInfo(hashCode);
	}
    ENGINE_CORE_API const TypeInfo* TypeRegistry_TryGetType(size_t hashCode){
        return ZTypeRegistry::TryGetTypeInfo(hashCode);
    }
}