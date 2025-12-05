#pragma once
#include "EngineCore/EngineCoreAPI.hpp"
#include "JobFunction.hpp"
#include "JobPriority.hpp"
#include "JobHandle.hpp"

using StateEngine::EngineCore::JobSystem::JobFunction;
using StateEngine::EngineCore::JobSystem::JobPriority;
using StateEngine::EngineCore::JobSystem::JobHandle;

extern "C" {
	ENGINE_CORE_API void JobSystem_ExecuteTask(JobFunction&& task, JobPriority priority = JobPriority::High, JobHandle* completionSignal = nullptr);
	ENGINE_CORE_API void JobSystem_ExecuteParallelFor(size_t totalItems, size_t itemsPerTask, JobFunction&& taskFunction, JobPriority priority = JobPriority::High, JobHandle* completionSignal = nullptr);
	ENGINE_CORE_API void JobSystem_WaitForJob(const JobHandle& jobHandle);
	ENGINE_CORE_API size_t JobSystem_GetPerfomanceWorkerCount();
	ENGINE_CORE_API size_t JobSystem_GetEfficiencyWorkerCount();
	ENGINE_CORE_API size_t JobSystem_GetTotalWorkerCount();
}
