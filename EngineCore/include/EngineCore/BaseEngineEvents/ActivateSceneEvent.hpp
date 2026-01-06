#pragma once
#include "EngineCore/DataStructures/ZString.hpp"
#include <cstdint>

namespace StateEngine::EngineCore::BaseEngineEvents {
    struct ActivateSceneEvent {
        DataStructures::ZString sceneName;
        uint32_t updateOrder {0};
        uint32_t renderOrder {0};
    };
}  // namespace StateEngine::EngineCore::BaseEngineEvents