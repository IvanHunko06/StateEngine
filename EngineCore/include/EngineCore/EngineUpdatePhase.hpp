#pragma once

namespace StateEngine::EngineCore {
	enum class EngineUpdatePhase {
        Input,
        PreLogic,
        Physics,
        PostLogic,
        Render
	};
}