#pragma once

namespace StateEngine::EngineCore {
    class IUpdatableSystem {
    public:
        virtual ~IUpdatableSystem() = default;
        virtual void UpdateSystem(float realDeltaTime) = 0;
    };
}