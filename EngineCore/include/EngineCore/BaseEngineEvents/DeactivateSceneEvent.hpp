#pragma once
#include "EngineCore/DataStructures/ZString.hpp"

namespace StateEngine::EngineCore::BaseEngineEvents {
    struct DeactivateSceneEvent {
        DataStructures::ZString sceneName;
    };
}  // namespace StateEngine::EngineCore::BaseEngineEvents