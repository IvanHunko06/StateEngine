#include "EngineCore/EngineTypeSystem/TypeRegistryExports.hpp"
#include "EngineCore/EngineCoreAPI.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineTypeSystem/ZTypeRegistry.hpp"
#include <cassert>
#include <utility>

using namespace StateEngine::EngineCore::EngineTypeSystem;
extern "C" {
ENGINE_CORE_API bool TypeRegistry_RegisterType(TypeInfo&& info)
{
    return ZTypeRegistry::RegisterType(std::move(info));
}
ENGINE_CORE_API void TypeRegistry_RemoveType(TypeKey hashCode)
{
    ZTypeRegistry::UnregisterType(hashCode);
}
ENGINE_CORE_API const TypeInfo* TypeRegistry_GetType(TypeKey hashCode)
{
    return ZTypeRegistry::GetTypeInfo(hashCode);
}
}