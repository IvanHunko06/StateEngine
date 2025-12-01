#pragma once
namespace StateEngine::EngineCore::MemoryManagment {
	class PageAllocator {
	private:
		static size_t pageSize_;
	public:
		PageAllocator() = delete;
		PageAllocator(const PageAllocator&) = delete;
		PageAllocator(PageAllocator&&) = delete;

		static void* reserve(size_t size);
		static bool tryCommit(void* addr, size_t size);
		static bool tryRelease(void* addr);
		static size_t getPageSize();
	};
}