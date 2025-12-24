#pragma once
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/DataStructures/ZString.hpp"
#include "EnumMember.hpp"
#include "FieldInfo.hpp"
#include <cstdint>

namespace StateEngine::EngineCore::EngineTypeSystem {
    enum class TypeKind {
        Primitive,
        Struct,
        Class,
        Interface,
        Enum,
    };
    using TypeKey = size_t;
    struct TypeInfo {
        DataStructures::ZString Name {};
        TypeKey HashCode {0};
        size_t Size {1};
        uint32_t Alignment {1};
        TypeKind kind {TypeKind::Primitive};

        using ToStringFunction = DataStructures::ZString (*)(const void* data, const char* format);
        ToStringFunction ToString {};

        using MoveConstructorFunction = void (*)(void* dstMemory, void* src);
        MoveConstructorFunction MoveConstructor {nullptr};

        using CopyConstructorFunction = void (*)(void* dstMemory, const void* src);
        CopyConstructorFunction CopyConstructor {nullptr};

        using DestructorFunction = void (*)(void* obj);
        DestructorFunction Destructor {nullptr};

        DataStructures::ZBuffer<FieldInfo> Fields {};
        DataStructures::ZBuffer<EnumMember> EnumMembers {};
    };
}  // namespace StateEngine::EngineCore::EngineTypeSystem