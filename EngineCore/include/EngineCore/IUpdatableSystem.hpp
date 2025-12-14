#pragma once

namespace StateEngine::EngineCore {
    class IEngineSystem {
    public:
        virtual ~IEngineSystem() = default;
        virtual void UpdateSystem(float realDeltaTime) = 0;
    };
}