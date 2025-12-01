#pragma once
#include <atomic>
#include <cstdint>
namespace StateEngine::EngineCore::Threading{
	template<size_t BITS>
	class ZAtomicBitset {
	private:
		static_assert(BITS > 0, "BITS must be greater than 0");
		static constexpr uint32_t GetDWordsCount() {
			return (BITS + 31) / 32; // round up
		}
		static constexpr uint32_t GetDWordByIndex(size_t idx) {
			return idx / 32;
		}
		std::atomic<uint32_t> dwords_[GetDWordsCount()]{ 0 };
	public:
		ZAtomicBitset() {
			clear();
		}
		void reset(size_t idx) {
			if (idx >= BITS) return;
			uint32_t dwordIndex = GetDWordByIndex(idx);
			uint32_t mask = 1u << (idx % 32);

			auto& currentDWord = dwords_[dwordIndex];
			uint32_t old = currentDWord.load(std::memory_order_relaxed);

			while (true) {
				if (!(old & mask)) return; // already reseet
				if (currentDWord.compare_exchange_weak(old, old & ~mask,
					std::memory_order_acq_rel,
					std::memory_order_relaxed)) {
					return;
				}
			}
		}
		void set(size_t idx) {
			if (idx >= BITS) return;
			int dwordIndex = GetDWordByIndex(idx);
			uint32_t mask = 1u << (idx % 32);

			auto& currentDWord = dwords_[dwordIndex];
			uint32_t old = currentDWord.load(std::memory_order_relaxed);

			while (true) {
				if (old & mask) return;// already set
				if (currentDWord.compare_exchange_weak(old, old | mask,
					std::memory_order_acq_rel,
					std::memory_order_relaxed)) {
					return;
				}
			}
		}
		bool test(size_t idx) const {
			if (idx >= BITS) return false;
			uint32_t dwordIndex = GetDWordByIndex(idx);
			uint32_t mask = 1u << (idx % 32);
			return dwords_[dwordIndex].load(std::memory_order_acquire) & mask;
		}
		void clear() {
			for (auto& w : dwords_) w.store(0, std::memory_order_relaxed);
		}
	};
}