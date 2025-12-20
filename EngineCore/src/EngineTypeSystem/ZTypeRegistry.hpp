#pragma once
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/DataStructures/ZString.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"

using StateEngine::EngineCore::DataStructures::ZHashMap;
using StateEngine::EngineCore::DataStructures::ZString;

namespace StateEngine::EngineCore::EngineTypeSystem {
    class ZTypeRegistry {
      private:
        static ZHashMap<size_t, TypeInfo> MapTypes;
        static bool IsSealed;

      public:
        static bool RegisterType(TypeInfo&& type);
        static const TypeInfo* TryGetTypeInfo(size_t hashCode);
        static const TypeInfo& GetRequiredTypeInfo(size_t hashCode);
        static void RemoveType(size_t hashCode);
        static void RegisterBaseTypes();
        static inline void SetIsSealed(bool newValue)
        {
            IsSealed = newValue;
        }
    };
}  // namespace StateEngine::EngineCore::EngineTypeSystem