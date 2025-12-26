#pragma once
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"

namespace StateEngine::EngineCore::EngineTypeSystem {
    class ZTypeRegistry {
      private:
        static DataStructures::ZHashMap<TypeKey, TypeInfo> MapTypes;
        static bool IsSealed;

      public:
        static bool RegisterType(TypeInfo&& type);
        static const TypeInfo* GetTypeInfo(TypeKey hashCode);
        static void UnregisterType(TypeKey hashCode);
        static void RegisterBaseTypes();
        static void UnregisterBaseTypes();
        static inline void SetIsSealed(bool newValue)
        {
            IsSealed = newValue;
        }
        static void ÑheckAllTypesRelease();
    };
}  // namespace StateEngine::EngineCore::EngineTypeSystem