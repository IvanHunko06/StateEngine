#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif
#include "EngineCore/DataStructures/ZBuffer.hpp"

using StateEngine::EngineCore::DataStructures::ZBuffer;
namespace StateEngine::EngineCore::Threading {
	class CpuCoresBinding {
	public:
		struct CpuCoreInfo {
			uint32_t physicalId;
			uint32_t logicalId;
			uint32_t coreId;
		};
	private:
		static ZBuffer<CpuCoreInfo> efficiencyCores_;
		static ZBuffer<CpuCoreInfo> perfomanceCores_;
		static ZBuffer<CpuCoreInfo> physicalCores_;
#ifdef _WIN32
		static bool useCpuSets_;
#endif
	public:
		static void CollectCores() noexcept;
		static void BindThreadToCore(const ZBuffer<uint32_t>& cores) noexcept;
		static inline const ZBuffer<CpuCoreInfo>& GetPerfomanceCores() noexcept {
			return perfomanceCores_;
		}
		static inline const ZBuffer<CpuCoreInfo>& GetEfficiencyCores() noexcept {
			return efficiencyCores_;
		}
		static inline const ZBuffer<CpuCoreInfo>& GetPhysicalCores() noexcept {
			return physicalCores_;
		}
		static inline bool HasEfficiencyCores() noexcept {
			return !efficiencyCores_.empty();
		}
		static inline const CpuCoreInfo& GetMainCoreId() noexcept {
			if (!perfomanceCores_.empty())
				return perfomanceCores_[0];
			return physicalCores_[0];	
		}
#ifdef _WIN32
		static inline void SetUseCpuSets(bool use) {
			useCpuSets_ = use;
		}
#endif
	private:
#ifdef _WIN32
		static bool TryCollectViaCpuSets() noexcept;
		static void CollectViaLegacy() noexcept;
		static bool TryBindViaCpuSets(HANDLE hThread, const ZBuffer<uint32_t>& cores) noexcept;
		static void BindViaAffinity(HANDLE hThread, const ZBuffer<uint32_t>& cores) noexcept;
#endif
	};
}