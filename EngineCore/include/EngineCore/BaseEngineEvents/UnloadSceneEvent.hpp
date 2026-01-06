#pragma once
#include "EngineCore/DataStructures/ZString.hpp"

namespace StateEngine::EngineCore::BaseEngineEvents {
    struct UnloadSceneEvent {
        DataStructures::ZString sceneName;
    };
}  // namespace StateEngine::EngineCore::BaseEngineEvents