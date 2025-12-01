#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif
#include "EngineCore/DataStructures/ZBuffer.hpp"

using StateEngine::EngineCore::DataStructures::ZBuffer;
namespace StateEngine::EngineCore::Threading {
	class CpuCoresBinding {
	private:
		static ZBuffer<uint32_t> efficiencyCores_;
		static ZBuffer<uint32_t> perfomanceCores_;
		static ZBuffer<uint32_t> physicalCores_;
#ifdef _WIN32
		static bool useCpuSets_;
#endif
	public:
		static void CollectCores() noexcept;
		static void BindThreadToCore(const ZBuffer<uint32_t>& cores) noexcept;
		static inline const ZBuffer<uint32_t>& GetPerfomanceCores() noexcept {
			return perfomanceCores_;
		}
		static inline const ZBuffer<uint32_t>& GetefficiencyCores() noexcept {
			return efficiencyCores_;
		}
		static inline const ZBuffer<uint32_t>& GetPhysicalCores() noexcept {
			return physicalCores_;
		}
		static inline bool HasEfficiencyCores() noexcept {
			return !efficiencyCores_.empty();
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