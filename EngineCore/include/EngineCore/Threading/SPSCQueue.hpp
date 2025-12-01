#pragma once
#include <atomic>
namespace StateEngine::EngineCore::Threading {

	template <typename TItem, size_t Size>
	class SPSCQueue {
	private:
		TItem buffer_[Size];
		alignas(64) std::atomic<size_t> head_{ 0 };
		alignas(64) std::atomic<size_t> tail_{ 0 };
	public:
		SPSCQueue(){
			for(size_t i = 0; i < Size; ++i){
				buffer_[i] = TItem();
			}
		}
		bool push(const TItem& item) {
			const size_t currentHead = head_.load(std::memory_order_relaxed);
			const size_t nextHead = (currentHead + 1) % Size;

			if (nextHead == tail_.load(std::memory_order_acquire))
				return false;

			buffer_[currentHead] = item;

			head_.store(nextHead, std::memory_order_release);
			return true;
		}
		bool pop(TItem& item) {
			const size_t currentTail = tail_.load(std::memory_order_relaxed);

			if (currentTail == head_.load(std::memory_order_acquire))
				return false;

			item = buffer_[currentTail];

			tail_.store((currentTail + 1) % Size, std::memory_order_release);
			return true;
		}
		void clear() {
			head_.store(0, std::memory_order_relaxed);
			tail_.store(0, std::memory_order_relaxed);
		}
	};
}