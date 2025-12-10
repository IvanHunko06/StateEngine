#include "ZJobSystem.hpp"
#include "EngineCore/DataStructures/ZHashSet.hpp"
#include <chrono>
#include <sstream>

using namespace StateEngine::EngineCore::JobSystem;
using namespace StateEngine::EngineCore::DataStructures;
using namespace StateEngine::EngineCore::Threading;

ZBuffer<ZJobSystem::WorkerThread> ZJobSystem::workerThreads_;
std::atomic<bool> ZJobSystem::isShuttingDown_;
std::array<BlockingMPMCQueue<ZJobSystem::JobInternal>, 4> ZJobSystem::jobQueues_;
thread_local bool ZJobSystem::isWorkerThread_;
thread_local JobPriority ZJobSystem::workerThreadPriority_ = JobPriority::High;
size_t ZJobSystem::PerfomanceWorkersCount;
size_t ZJobSystem::EfficiencyWorkersCount;

void ZJobSystem::Initialize() {
	workerThreads_.clear();
	isShuttingDown_.store(false, std::memory_order_relaxed);
	ZBuffer<uint32_t> higestPriorityCores = GetCoresForPriority(JobPriority::Highest);
	ZBuffer<uint32_t> highPriorityCores = GetCoresForPriority(JobPriority::High);
	ZBuffer<uint32_t> lowPriorityCores = GetCoresForPriority(JobPriority::Low);
	ZBuffer<uint32_t> lowestPriorityCores = GetCoresForPriority(JobPriority::Lowest);

	auto& pCoreInfos = CpuCoresBinding::GetPerfomanceCores();
	auto& eCoreInfos = CpuCoresBinding::GetEfficiencyCores();
	auto& physicalCores = CpuCoresBinding::GetPhysicalCores();
	auto& mainCoreId = CpuCoresBinding::GetMainCoreId();

	PerfomanceWorkersCount = highPriorityCores.size();
	EfficiencyWorkersCount = lowestPriorityCores.size();
	workerThreads_.reserve(PerfomanceWorkersCount + EfficiencyWorkersCount);

	uint16_t threadIndex = 0;
	if (CpuCoresBinding::HasEfficiencyCores()) {
		for (auto& core : pCoreInfos) {
			if(core.physicalId == mainCoreId.physicalId) continue; // Skip main thread core
			WorkerThread worker;
			if(core.physicalId == core.logicalId) {
				worker.priority = JobPriority::Highest;
				worker.assignedCores = higestPriorityCores;
			}
			else {
				worker.priority = JobPriority::High;
				worker.assignedCores = highPriorityCores;
			}
			worker.threadIndex = threadIndex++;
			workerThreads_.push_back(std::move(worker));
			workerThreads_.back().thread = std::thread(&ZJobSystem::WorkerThreadMain,
														std::ref(workerThreads_.back()));
		}
		auto& lastECore = eCoreInfos.back();
		for(auto& core : eCoreInfos) {
			if (core.physicalId == lastECore.physicalId) continue; // Skip last E-core for OS
			WorkerThread worker;
			if(core.physicalId == core.logicalId) {
				worker.priority = JobPriority::Low;
				worker.assignedCores = lowPriorityCores;
			}
			else {
				worker.priority = JobPriority::Lowest;
				worker.assignedCores = lowestPriorityCores;
			}
			worker.threadIndex = threadIndex++;
			workerThreads_.push_back(std::move(worker));
			workerThreads_.back().thread = std::thread(&ZJobSystem::WorkerThreadMain,
														std::ref(workerThreads_.back()));
		}
	}
	else {
		for (auto& core : higestPriorityCores) {
			WorkerThread worker;
			worker.priority = JobPriority::Highest;
			worker.assignedCores = higestPriorityCores;
			worker.threadIndex = threadIndex++;
			workerThreads_.push_back(std::move(worker));
			workerThreads_.back().thread = std::thread(&ZJobSystem::WorkerThreadMain,
														std::ref(workerThreads_.back()));
		}
		for (auto& core : lowestPriorityCores) {
			WorkerThread worker;
			worker.priority = JobPriority::Lowest;
			worker.assignedCores = lowestPriorityCores;
			worker.threadIndex = threadIndex++;
			workerThreads_.push_back(std::move(worker));
			workerThreads_.back().thread = std::thread(&ZJobSystem::WorkerThreadMain,
														std::ref(workerThreads_.back()));
		}
	}

}
void ZJobSystem::Shutdown() {
	isShuttingDown_.store(true);
	for (auto& worker : workerThreads_) {
		jobQueues_[static_cast<size_t>(worker.priority)].enqueue({}); // Wake up thread waiting for job in case it's idle
		if (worker.thread.joinable()) {
			worker.thread.join();
		}
	}
}

ZBuffer<uint32_t> ZJobSystem::GetCoresForPriority(JobPriority priority) {
	auto intersect = [](const ZBuffer<uint32_t>& a, const ZBuffer<uint32_t>& b) -> ZBuffer<uint32_t> {
		ZBuffer<uint32_t> result;
		ZHashSet<uint32_t> setB;
		for (auto v : b) setB.insert(v);
		for (auto v : a) {
			if (setB.contains(v)) result.push_back(v);
		}
		return result;
	};
	auto difference = [](const ZBuffer<uint32_t>& a, const ZBuffer<uint32_t>& b) -> ZBuffer<uint32_t> {
		ZBuffer<uint32_t> result;
		ZHashSet<uint32_t> setB;
		for (auto v : b) setB.insert(v);
		for (auto v : a) {
			if (!setB.contains(v)) result.push_back(v);
		}
		return result;
	};
	
	auto& physicalCoresInfos = CpuCoresBinding::GetPhysicalCores();
	ZBuffer<uint32_t> physicalCores;
	for (auto& coreInfo : physicalCoresInfos) {
		physicalCores.push_back(coreInfo.coreId);
	}

	if (priority == JobPriority::Highest || priority == JobPriority::High) {
		ZBuffer<uint32_t> perfomanceCores;
		auto& mainThreadCore = CpuCoresBinding::GetMainCoreId();
		const auto& pCoresInfos = CpuCoresBinding::GetPerfomanceCores();
		int physicalMainCoreIndex = -1;
		for(auto& coreInfo : pCoresInfos) {
			if (coreInfo.physicalId == mainThreadCore.physicalId) {
				physicalMainCoreIndex = coreInfo.physicalId;
				continue;
			}
			perfomanceCores.push_back(coreInfo.coreId);
		}
		if (CpuCoresBinding::HasEfficiencyCores()) {
			return priority == JobPriority::Highest ? intersect(perfomanceCores, physicalCores) : perfomanceCores;
		}
		else { // Use physical cores only if no E-cores
			return intersect(perfomanceCores, physicalCores);
		}
		
	}
	
	if (CpuCoresBinding::HasEfficiencyCores()) {
		const auto& eCoresInfos = CpuCoresBinding::GetEfficiencyCores();
		ZBuffer<uint32_t> efficiencyCores;
		for (int i = 0; i < eCoresInfos.size() - 1; ++i) { // Skip last E-core for OS
			auto& coreInfo = eCoresInfos[i];
			efficiencyCores.push_back(coreInfo.coreId);
		}
		return priority == JobPriority::Low ? intersect(efficiencyCores, physicalCores) : efficiencyCores;
	}
	else { //Use logical cores as Low and Lowest priorities if no E-cores
		ZBuffer<uint32_t> logicalCores;
		auto& mainThreadCore = CpuCoresBinding::GetMainCoreId();
		const auto& lCoresInfos = CpuCoresBinding::GetPerfomanceCores();
		int physicalMainCoreIndex = -1;
		for (auto& coreInfo : lCoresInfos) {
			if (coreInfo.physicalId == mainThreadCore.physicalId)
				continue;
			logicalCores.push_back(coreInfo.coreId);
		}
		return difference(logicalCores, physicalCores);

	}
}

inline const char* GetJobPriorityName(JobPriority priority) {
	switch (priority) {
	case JobPriority::Highest:
		return "Highest";
	case JobPriority::High:
		return "High";
	case JobPriority::Low:
		return "Low";
	case JobPriority::Lowest:
		return "Lowest";
	default:
		return "Unknown";
	}
}

void ZJobSystem::WorkerThreadMain(const WorkerThread& worker) {
	CpuCoresBinding::BindThreadToCore(worker.assignedCores);
#ifdef _WIN32
	std::wstringstream stream;
	stream << L"JobSystem Worker Thread  " << worker.threadIndex << L" (" << GetJobPriorityName(worker.priority) << L")";
	SetThreadDescription(GetCurrentThread(), stream.str().c_str());
#endif
	isWorkerThread_ = true;
	workerThreadPriority_ = worker.priority;
	while (!isShuttingDown_.load(std::memory_order_relaxed)) {
		ExecuteNextJob(worker.priority);
	}
}

std::optional<ZJobSystem::JobInternal> ZJobSystem::FetchJob(JobPriority threadPriority) {
	std::chrono::duration<size_t, std::micro> kWaitTimeout(500);
	if(threadPriority == JobPriority::Highest) {
		JobInternal job;
		if (jobQueues_[static_cast<size_t>(JobPriority::Highest)].wait_dequeue_timed(job, kWaitTimeout)) {
			return job;
		}
		if (jobQueues_[static_cast<size_t>(JobPriority::High)].try_dequeue(job)) {
			return job;
		}
	}
	else if (threadPriority == JobPriority::High) {
		JobInternal job;
		if (jobQueues_[static_cast<size_t>(JobPriority::High)].wait_dequeue_timed(job, kWaitTimeout)) {
			return job;
		}
		if (jobQueues_[static_cast<size_t>(JobPriority::Highest)].try_dequeue(job)) {
			return job;
		}
		if (jobQueues_[static_cast<size_t>(JobPriority::Low)].try_dequeue(job)) {
			return job;
		}
		if (jobQueues_[static_cast<size_t>(JobPriority::Lowest)].try_dequeue(job)) {
			return job;
		}
	}
	else if (threadPriority == JobPriority::Low) {
		JobInternal job;
		if (jobQueues_[static_cast<size_t>(JobPriority::Low)].wait_dequeue_timed(job, kWaitTimeout)) {
			return job;
		}
		if (jobQueues_[static_cast<size_t>(JobPriority::Lowest)].try_dequeue(job)) {
			return job;
		}
	}
	else if (threadPriority == JobPriority::Lowest) {
		JobInternal job;
		if (jobQueues_[static_cast<size_t>(JobPriority::Lowest)].wait_dequeue_timed(job, kWaitTimeout)) {
			return job;
		}
		if (jobQueues_[static_cast<size_t>(JobPriority::Low)].try_dequeue(job)) {
			return job;
		}
	}
	return std::nullopt;
}

void ZJobSystem::ExecuteTask(JobFunction&& task, JobPriority priority, JobHandle* completionSignal) {
	if (!task) {
		assert(false && "ZJobSystem::ExecuteTask called with empty task");
		return;
	}
	JobInternal jobInternal{
		.function = std::move(task),
		.rangeStart = 0,
		.rangeEnd = 1,
		.completionHandle = completionSignal
	};
	if (completionSignal) {
		completionSignal->remainingSubtasks.store(1, std::memory_order_relaxed);
		completionSignal->isCompleted.store(false, std::memory_order_relaxed);
	}

	jobQueues_[static_cast<size_t>(priority)].enqueue(std::move(jobInternal));
}
void ZJobSystem::ExecuteParallelFor(size_t totalItems, size_t itemsPerTask, JobFunction&& taskFunction, JobPriority priority, JobHandle* completionSignal) {
	if(!taskFunction){
		assert(false && "ZJobSystem::ExecuteParallelFor called with empty taskFunction");
		return;
	}
	if(!totalItems || !itemsPerTask){
		assert(false && "ZJobSystem::ExecuteParallelFor called with zero totalItems or itemsPerTask");
		return;
	}
	size_t totalTasks = (totalItems + itemsPerTask - 1) / itemsPerTask;
	for(size_t i = 0; i < totalTasks; ++i) {
		size_t rangeStart = i * itemsPerTask;
		size_t rangeEnd = std::min(rangeStart + itemsPerTask, totalItems);
		JobInternal jobInternal{
			.function = taskFunction,
			.rangeStart = rangeStart,
			.rangeEnd = rangeEnd,
			.completionHandle = completionSignal
		};
		if (completionSignal) {
			completionSignal->remainingSubtasks.fetch_add(1, std::memory_order_relaxed);
			completionSignal->isCompleted.store(false, std::memory_order_relaxed);
		}
		jobQueues_[static_cast<size_t>(priority)].enqueue(std::move(jobInternal));
	}

}
void ZJobSystem::WaitForJob(const JobHandle& jobHandle) {
	while (!jobHandle.isCompleted.load(std::memory_order_acquire) && !isShuttingDown_.load(std::memory_order_relaxed)){
		if (isWorkerThread_)
			ExecuteNextJob(workerThreadPriority_);
		else
			ExecuteNextJob(JobPriority::High);
	}
}

bool ZJobSystem::ExecuteNextJob(JobPriority threadPriority) {
	auto jobOpt = FetchJob(threadPriority);
	if (!jobOpt.has_value()) return false;
	JobInternal job = jobOpt.value();
	if (!job.function) return true; // Empty job used to wake up thread during shutdown
	job.function(job.rangeStart, job.rangeEnd);
	if (job.completionHandle) {
		uint32_t oldValue = job.completionHandle->remainingSubtasks.fetch_sub(1, std::memory_order_acq_rel);
		if (oldValue == 1) {
			job.completionHandle->isCompleted.store(true, std::memory_order_release);
		}
	}
	return true;
}