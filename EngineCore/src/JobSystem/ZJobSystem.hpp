#pragma once
#include "EngineCore/JobSystem/JobPriority.hpp"
#include "EngineCore/JobSystem/JobFunction.hpp"
#include "EngineCore/JobSystem/JobHandle.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "Threading/CpuCoresBinding.hpp"
#include "EngineCore/Threading/BlockingMPMCQueue.hpp"
#include <optional>

#include <thread>
using StateEngine::EngineCore::Threading::CpuCoresBinding;
using StateEngine::EngineCore::Threading::BlockingMPMCQueue;
namespace StateEngine::EngineCore::JobSystem {
	class ZJobSystem {
	private:
		struct WorkerThread {
			std::thread thread;
			JobPriority priority;
			uint16_t threadIndex;
			ZBuffer<uint32_t> assignedCores;		
		};
		struct JobInternal {
			JobFunction function;
			size_t rangeStart;
			size_t rangeEnd;
			JobHandle* completionHandle;
		};
		static ZBuffer<WorkerThread> workerThreads_;
		static std::atomic<bool> isShuttingDown_;
		static std::array<BlockingMPMCQueue<JobInternal>, 4> jobQueues_; // One queue per priority level
		static thread_local bool isWorkerThread_;
		static thread_local JobPriority workerThreadPriority_;
		static size_t PerfomanceWorkersCount;
		static size_t EfficiencyWorkersCount;
	public:
		static void Initialize();
		static void Shutdown();
		static void ExecuteTask(JobFunction&& task, JobPriority priority = JobPriority::High, JobHandle* completionSignal = nullptr);
		static void ExecuteParallelFor(size_t totalItems, size_t itemsPerTask, JobFunction&& taskFunction, JobPriority priority = JobPriority::High, JobHandle* completionSignal = nullptr);
		static void WaitForJob(const JobHandle& jobHandle);
		static inline size_t GetPerfomanceWorkerCount() {
			return PerfomanceWorkersCount;
		}
		static inline size_t GetEfficiencyWorkerCount() {
			return EfficiencyWorkersCount;
		}
		static inline size_t GetTotalWorkerCount() {
			return PerfomanceWorkersCount + EfficiencyWorkersCount;
		}
	private:
		static ZBuffer<uint32_t> GetCoresForPriority(JobPriority priority);
		static bool ExecuteNextJob(JobPriority threadPriority);
		static void WorkerThreadMain(const WorkerThread& worker);
		static std::optional<JobInternal> FetchJob(JobPriority threadPriority);
	};
}