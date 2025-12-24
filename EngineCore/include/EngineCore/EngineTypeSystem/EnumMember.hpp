#pragma once
#include <cstdint>
#include "EngineCore/DataStructures/ZString.hpp"
namespace StateEngine::EngineCore::EngineTypeSystem {
    struct EnumMember {
        int32_t NumberValue {0};
        DataStructures::ZString StringValue {nullptr};
        EnumMember(const char* strVal, uint32_t numVal) : NumberValue(numVal), StringValue(strVal) {}
        EnumMember() = default;
    };
}  // namespace StateEngine::EngineCore::EngineTypeSystem