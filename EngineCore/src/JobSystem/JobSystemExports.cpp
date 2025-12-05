#include "EngineCore/JobSystem/JobSystemExports.hpp"
#include "ZJobSystem.hpp"
using StateEngine::EngineCore::JobSystem::ZJobSystem;
extern "C" {
	ENGINE_CORE_API void JobSystem_ExecuteTask(JobFunction&& task, JobPriority priority, JobHandle* completionSignal) {
		ZJobSystem::ExecuteTask(std::move(task), priority, completionSignal);
	}
	ENGINE_CORE_API void JobSystem_ExecuteParallelFor(size_t totalItems, size_t itemsPerTask, JobFunction&& taskFunction, JobPriority priority, JobHandle* completionSignal) {
		ZJobSystem::ExecuteParallelFor(totalItems, itemsPerTask, std::move(taskFunction), priority, completionSignal);
	}
	ENGINE_CORE_API void JobSystem_WaitForJob(const JobHandle& jobHandle) {
		ZJobSystem::WaitForJob(jobHandle);
	}
	ENGINE_CORE_API size_t JobSystem_GetPerfomanceWorkerCount() {
		return ZJobSystem::GetPerfomanceWorkerCount();
	}
	ENGINE_CORE_API size_t JobSystem_GetEfficiencyWorkerCount() {
		return ZJobSystem::GetEfficiencyWorkerCount();
	}
	ENGINE_CORE_API size_t JobSystem_GetTotalWorkerCount() {
		return ZJobSystem::GetTotalWorkerCount();
	}
}
