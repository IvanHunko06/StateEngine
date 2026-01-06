#pragma once
#include "EngineCore/DataStructures/ZString.hpp"

namespace StateEngine::EngineCore::BaseEngineEvents {
    struct LoadSceneEvent {
        DataStructures::ZString sceneName;
        bool loadAsync {true};
    };
}  // namespace StateEngine::EngineCore::BaseEngineEvents