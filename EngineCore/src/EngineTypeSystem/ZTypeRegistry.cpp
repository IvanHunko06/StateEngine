#include "ZTypeRegistry.hpp"
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/EngineTypeSystem/CompileTimeTypeBuilder.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistry.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "Logging/ZLogger.hpp"
#include <cassert>
#include <cstdint>
#include <utility>
#include <string>
using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::EngineCore::DataStructures;
using namespace StateEngine::EngineCore::Logging;

ZHashMap<TypeKey, TypeInfo> ZTypeRegistry::MapTypes;
bool ZTypeRegistry::IsSealed = false;

bool ZTypeRegistry::RegisterType(TypeInfo&& type)
{
    if (IsSealed) {
        assert(!IsSealed && "Addition is only allowed in the RegisterTypes function.");
        return false;
    }
    if (MapTypes.contains(type.HashCode)) {
        const TypeInfo& registeredType = MapTypes[type.HashCode];
        ZLOG_WARN("EngineCore") << "A hash collision was detected. The new type's " << type.Name.c_str()
                                << "hash matches an already registered type " << registeredType.Name.c_str()
                                << ". The new type will not be added.";
        return false;
    }

    MapTypes[type.HashCode] = std::move(type);
    return true;
}
const TypeInfo* ZTypeRegistry::GetTypeInfo(TypeKey hashCode)
{
    auto it = MapTypes.find(hashCode);
    if (it == MapTypes.end()) {
        return nullptr;
    }
    return &it->second;
}

void ZTypeRegistry::UnregisterType(TypeKey hashCode)
{
    if (IsSealed) {
        assert(!IsSealed && "Deletion is only allowed in the UnregisterTypes function.");
        return;
    }

    MapTypes.erase(hashCode);
}

void ZTypeRegistry::RegisterBaseTypes()
{
    TypeRegistry::RegisterType(CompileTimeTypeBuilder<void>::Build(), TypeKind::Primitive);
    TypeRegistry::RegisterType(CompileTimeTypeBuilder<bool>::Build(), TypeKind::Primitive);
    TypeRegistry::RegisterType(CompileTimeTypeBuilder<char>::Build(), TypeKind::Primitive);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<int8_t>::Build(), TypeKind::Primitive);
    TypeRegistry::RegisterType(CompileTimeTypeBuilder<uint8_t>::Build(), TypeKind::Primitive);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<int16_t>::Build(), TypeKind::Primitive);
    TypeRegistry::RegisterType(CompileTimeTypeBuilder<uint16_t>::Build(), TypeKind::Primitive);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<int32_t>::Build(), TypeKind::Primitive);
    TypeRegistry::RegisterType(CompileTimeTypeBuilder<uint32_t>::Build(), TypeKind::Primitive);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<int64_t>::Build(), TypeKind::Primitive);
    TypeRegistry::RegisterType(CompileTimeTypeBuilder<uint64_t>::Build(), TypeKind::Primitive);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<float>::Build(), TypeKind::Primitive);
    TypeRegistry::RegisterType(CompileTimeTypeBuilder<double>::Build(), TypeKind::Primitive);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<DataStructures::ZString>::Build(), TypeKind::Class);
}

void ZTypeRegistry::UnregisterBaseTypes() {
    TypeRegistry::UnregisterType<void>();
    TypeRegistry::UnregisterType<bool>();
    TypeRegistry::UnregisterType<char>();

    TypeRegistry::UnregisterType<int8_t>();
    TypeRegistry::UnregisterType<uint8_t>();

    TypeRegistry::UnregisterType<int16_t>();
    TypeRegistry::UnregisterType<uint16_t>();

    TypeRegistry::UnregisterType<int32_t>();
    TypeRegistry::UnregisterType<uint32_t>();

    TypeRegistry::UnregisterType<int64_t>();
    TypeRegistry::UnregisterType<uint64_t>();

    TypeRegistry::UnregisterType<float>();
    TypeRegistry::UnregisterType<double>();

    TypeRegistry::UnregisterType<DataStructures::ZString>();
}

void ZTypeRegistry::ÑheckAllTypesRelease() {
    for (const auto& type : MapTypes) {
        std::string errorMessage = "A type was found that was not freed: ";
        errorMessage += type.second.Name.c_str();

        assert(false && "A type was found that was not freed. An exception is possible.");
        ZLOG_ERROR("ZTypeRegistry") << errorMessage;
    }
    ZLogger::FlushMessages();
}