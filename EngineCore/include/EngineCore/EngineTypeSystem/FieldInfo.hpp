#pragma once
#include "EngineCore/DataStructures/ZString.hpp"

namespace StateEngine::EngineCore::EngineTypeSystem {
    struct TypeInfo;
    struct FieldInfo {
        DataStructures::ZString Name {};
        const TypeInfo* Type {nullptr};
        size_t TypeHashCode {0};
        size_t Offset {0};
    };
}  // namespace StateEngine::EngineCore::EngineTypeSystem