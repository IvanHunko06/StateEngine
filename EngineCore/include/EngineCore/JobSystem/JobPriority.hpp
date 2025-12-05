#pragma once

namespace StateEngine::EngineCore::JobSystem {
	enum class JobPriority{
		Highest,  // Only physical P-cores
		High,     // All P-cores
		Low,      // Only physical E-cores
		Lowest    // All E-cores
	};
}