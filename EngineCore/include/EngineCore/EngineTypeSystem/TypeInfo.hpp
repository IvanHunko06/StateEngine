#pragma once
#include "EngineCore/DataStructures/ZFunction.hpp"
#include "EngineCore/DataStructures/ZString.hpp"
#include "EnumMember.hpp"
#include "FieldInfo.hpp"
#include "MethodInfo.hpp"
#include "TypeKind.hpp"
#include <cstdint>

using StateEngine::EngineCore::DataStructures::ZFunction;
using StateEngine::EngineCore::DataStructures::ZString;
namespace StateEngine::EngineCore::EngineTypeSystem {
    struct TypeInfo {
        ZString Name {};
        size_t HashCode {0};
        size_t Size {1};
        uint32_t Alignment {1};
        TypeKind kind {TypeKind::Primitive};

        using ToStringFunction = ZString(*)(const void* data, const char* format);
        ToStringFunction ToString {};

        using MoveConstructorFunction = void (*)(void* dstMemory, void* src);
        MoveConstructorFunction MoveConstructor {nullptr};

        using CopyConstructorFunction = void (*)(void* dstMemory, const void* src);
        CopyConstructorFunction CopyConstructor {nullptr};

        using DestructorFunction = void (*)(void* obj);
        DestructorFunction Destructor {nullptr};

        ZBuffer<FieldInfo> Fields {};
        ZBuffer<EnumMember> EnumMembers {};

        TypeInfo() = default;
    };
}  // namespace StateEngine::EngineCore::EngineTypeSystem