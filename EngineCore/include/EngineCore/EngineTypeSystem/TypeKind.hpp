#pragma once

namespace StateEngine::EngineCore::EngineTypeSystem {
    enum class TypeKind {
        Primitive,
        Struct,
        Class,
        Interface,
        Enum,
    };
}
