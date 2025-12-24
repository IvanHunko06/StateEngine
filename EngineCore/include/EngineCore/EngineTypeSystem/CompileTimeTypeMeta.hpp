#pragma once
#include <array>
#include <cstddef>
#include <string_view>
#include "EngineCore/DataStructures/ZString.hpp"

namespace StateEngine::EngineCore::EngineTypeSystem {
    struct CompileTimeFieldDesc {
        std::string_view Name {};
        size_t Offset {0};
        size_t Size {0};
        size_t TypeHash {0};
        size_t FieldHash {0};
        size_t (*CalculateOffset)() = nullptr;
    };

    struct CompileTimeEnumItemDesc {
        std::string_view Name {};
        int32_t Value {0};
    };

    template <size_t N_Fields, size_t N_EnumItems>
    struct CompileTimeTypeMeta {
        std::string_view TypeName {};
        size_t TypeHash {0};
        size_t Size {0};
        size_t Alignment {0};

        std::array<CompileTimeFieldDesc, N_Fields> Fields {};
        std::array<CompileTimeEnumItemDesc, N_EnumItems> EnumItems {};

        size_t FieldCount {0};
        size_t EnumCount {0};

        DataStructures::ZString (*ToString)(const void* obj, const char* format) {nullptr};
        void (*CopyConstructor)(void* dst, const void* src) {nullptr};
        void (*MoveConstructor)(void* dst, void* src) {nullptr};
        void (*Destructor)(void* obj) {nullptr};
    };
}  // namespace StateEngine::EngineCore::EngineTypeSystem