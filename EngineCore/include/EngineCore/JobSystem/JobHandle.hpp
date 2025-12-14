#pragma once
#include <atomic>

namespace StateEngine::EngineCore::JobSystem {
	struct JobHandle {
		std::atomic_bool isCompleted{ false };
		std::atomic_bool isCanceled{ false };
		std::atomic<uint32_t> remainingSubtasks{ 0 };
	};
}