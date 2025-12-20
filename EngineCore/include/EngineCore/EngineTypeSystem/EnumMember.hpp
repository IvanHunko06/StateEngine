#pragma once
#include <cstdint>
#include "EngineCore/DataStructures/ZString.hpp"
using StateEngine::EngineCore::DataStructures::ZString;
namespace StateEngine::EngineCore::EngineTypeSystem {
    struct EnumMember {
        int32_t NumberValue {0};
        ZString StringValue {nullptr};
        EnumMember(const char* strVal, uint32_t numVal) : NumberValue(numVal), StringValue(strVal) {}
        EnumMember() = default;
    };
}  // namespace StateEngine::EngineCore::EngineTypeSystem