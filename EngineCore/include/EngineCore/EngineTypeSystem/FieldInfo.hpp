#pragma once
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/DataStructures/ZString.hpp"

using namespace StateEngine::EngineCore::DataStructures;

namespace StateEngine::EngineCore::EngineTypeSystem {
    struct TypeInfo;
    struct FieldInfo {
        ZString Name {};
        const TypeInfo* Type {nullptr};
        size_t TypeHashCode {0};
        size_t Offset {0};
    };
}  // namespace StateEngine::EngineCore::EngineTypeSystem