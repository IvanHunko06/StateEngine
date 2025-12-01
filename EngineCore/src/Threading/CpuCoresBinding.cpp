#include "CpuCoresBinding.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/DataStructures/ZHashSet.hpp"

using namespace StateEngine::EngineCore::Threading;
using StateEngine::EngineCore::DataStructures::ZHashSet;

ZBuffer<uint32_t> CpuCoresBinding::efficiencyCores_;
ZBuffer<uint32_t> CpuCoresBinding::perfomanceCores_;
ZBuffer<uint32_t> CpuCoresBinding::physicalCores_;
#ifdef _WIN32
bool CpuCoresBinding::useCpuSets_;
using PFN_GetSystemCpuSetInformation = BOOL(WINAPI*)(
	PSYSTEM_CPU_SET_INFORMATION Information,
	ULONG BufferLength,
	PULONG ReturnedLength,
	HANDLE Process,
	ULONG Flags
	);

using PFN_SetThreadSelectedCpuSets = BOOL(WINAPI*)(
	HANDLE Thread,
	const ULONG* CpuSetIds,
	ULONG CpuSetIdCount
	);
#endif

void CpuCoresBinding::CollectCores() noexcept {
	efficiencyCores_.clear();
	perfomanceCores_.clear();
	physicalCores_.clear();
#ifdef _WIN32
	ZLOG_DEBUG("EngineCore") << "Collecting CPU cores info. UseCpuSets: " << useCpuSets_;

	if (useCpuSets_) {
		if (TryCollectViaCpuSets()) {
			return;
		}
		ZLOG_WARN("EngineCore") << "CpuSets failed or not supported. Fallback to legacy method.";
	}

	CollectViaLegacy();
#endif
}
void CpuCoresBinding::BindThreadToCore(const ZBuffer<uint32_t>& cores) noexcept {
	if (cores.empty()) return;

#ifdef _WIN32
	HANDLE hThread = GetCurrentThread();

	if (useCpuSets_) {
		if (TryBindViaCpuSets(hThread, cores)) {
			return;
		}
	}

	BindViaAffinity(hThread, cores);
#endif
}

#ifdef _WIN32
bool CpuCoresBinding::TryCollectViaCpuSets() noexcept {
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) return false;

    auto func = reinterpret_cast<PFN_GetSystemCpuSetInformation>(
        GetProcAddress(hKernel32, "GetSystemCpuSetInformation")
        );

    if (!func) return false;

    ULONG bufferLen = 0;
    func(nullptr, 0, &bufferLen, GetCurrentProcess(), 0);

    if (bufferLen == 0) return false;

    auto* rawBuffer = MemoryAllocator_AlignedAllocate(bufferLen, alignof(SYSTEM_CPU_SET_INFORMATION));
    if (!rawBuffer) return false;

    uint8_t* buffer = reinterpret_cast<uint8_t*>(rawBuffer);

    if (!func(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer), bufferLen, &bufferLen, GetCurrentProcess(), 0)) {
        MemoryAllocator_Deallocate(rawBuffer);
        return false;
    }

    uint8_t* ptr = buffer;
    uint8_t* endPtr = buffer + bufferLen;
    ZHashSet<DWORD> seenPhysicalCoreIndices;

    while (ptr < endPtr) {
        auto info = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(ptr);

        if (info->Type == CpuSetInformation) {
            uint16_t id = static_cast<uint16_t>(info->CpuSet.Id);

            if (info->CpuSet.EfficiencyClass > 0) {
                perfomanceCores_.push_back(id);
            }
            else {
                efficiencyCores_.push_back(id);
            }

            if (seenPhysicalCoreIndices.find(info->CpuSet.CoreIndex) == seenPhysicalCoreIndices.end()) {
                physicalCores_.push_back(id);
                seenPhysicalCoreIndices.insert(info->CpuSet.CoreIndex);
            }
        }
        ptr += info->Size;
    }

    MemoryAllocator_Deallocate(rawBuffer);

    ZLOG_DEBUG("EngineCore") << "CpuSets collected. P-Cores: " << perfomanceCores_.size()
        << ", E-Cores: " << efficiencyCores_.size();
    return true;
}

void CpuCoresBinding::CollectViaLegacy() noexcept {
    ZLOG_DEBUG("EngineCore") << "Using GetLogicalProcessorInformationEx (Legacy/Fallback).";

    DWORD len = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &len);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || len == 0) {
        ZLOG_ERROR("EngineCore") << "GetLogicalProcessorInformationEx init failed.";
        return;
    }

    auto* rawBuffer = MemoryAllocator_AlignedAllocate(len, alignof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX));
    if (!rawBuffer) return;

    uint8_t* buffer = reinterpret_cast<uint8_t*>(rawBuffer);

    if (GetLogicalProcessorInformationEx(RelationProcessorCore, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer), &len)) {
        uint8_t* ptr = buffer;
        uint8_t* endPtr = buffer + len;

        while (ptr < endPtr) {
            auto info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);

            if (info->Relationship == RelationProcessorCore) {
                ULONG_PTR mask = info->Processor.GroupMask[0].Mask;
                bool isFirstInCore = true;

                for (uint16_t i = 0; i < sizeof(ULONG_PTR) * 8; ++i) {
                    if ((mask >> i) & 1) {
                        perfomanceCores_.push_back(i);

                        if (isFirstInCore) {
                            physicalCores_.push_back(i);
                            isFirstInCore = false;
                        }
                    }
                }
            }
            ptr += info->Size;
        }

        efficiencyCores_ = perfomanceCores_;
        ZLOG_DEBUG("EngineCore") << "Legacy info collected. Physical Cores: " << physicalCores_.size();
    }

    MemoryAllocator_Deallocate(rawBuffer);
}


bool CpuCoresBinding::TryBindViaCpuSets(HANDLE hThread, const ZBuffer<uint32_t>& cores) noexcept {
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) return false;

    auto setFunc = reinterpret_cast<PFN_SetThreadSelectedCpuSets>(
        GetProcAddress(hKernel32, "SetThreadSelectedCpuSets")
        );

    if (!setFunc) return false;

    if (setFunc(hThread, reinterpret_cast<const ULONG*>(cores.data()), static_cast<ULONG>(cores.size()))) {
        return true;
    }
    else {
        ZLOG_ERROR("EngineCore") << "SetThreadSelectedCpuSets failed code: " << GetLastError();
        return false;
    }
}

void CpuCoresBinding::BindViaAffinity(HANDLE hThread, const ZBuffer<uint32_t>& cores) noexcept {
    DWORD_PTR mask = 0;
    for (auto id : cores) {
        if (id < 64) {
            mask |= (static_cast<DWORD_PTR>(1) << id);
        }
    }

    if (mask == 0) return;

    if (SetThreadAffinityMask(hThread, mask) == 0) {
        ZLOG_ERROR("EngineCore") << "SetThreadAffinityMask failed code: " << GetLastError();
    }
}
#endif