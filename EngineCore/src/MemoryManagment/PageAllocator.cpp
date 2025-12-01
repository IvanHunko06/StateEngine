#include "PageAllocator.hpp"
#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#endif

using namespace StateEngine::EngineCore::MemoryManagment;
size_t PageAllocator::pageSize_ = 0;

void* PageAllocator::reserve(size_t size) {
#ifdef _WIN32
	void* ptr = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
	return ptr;
#elif defined(__linux__) || defined(__APPLE__)
	void* ptr = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == MAP_FAILED)
		return nullptr;
	return ptr;
#else
#error "Unsupported platform"
#endif
}
bool PageAllocator::tryCommit(void* addr, size_t size) {
#ifdef _WIN32
	void* result = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
	return result != nullptr;
#elif defined(__linux__) || defined(__APPLE__)
	if (mprotect(addr, size, PROT_READ | PROT_WRITE) == 0)
		return true;
	return false;
#else
#error "Unsupported platform"
#endif
}
bool PageAllocator::tryRelease(void* addr) {
#ifdef _WIN32
	return VirtualFree(addr, 0, MEM_RELEASE) != 0;
#elif defined(__linux__) || defined(__APPLE__)
	return munmap(addr, getPageSize()) == 0;
#else
#error "Unsupported platform"
#endif
}
size_t PageAllocator::getPageSize() {
#ifdef _WIN32
	if (!pageSize_) {
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		pageSize_ = sysInfo.dwPageSize;
	}
	return pageSize_;
#elif defined(__linux__) || defined(__APPLE__)
	if (!pageSize_) {
		pageSize_ = static_cast<size_t>(sysconf(_SC_PAGESIZE));
	}
	return pageSize_;
#else
#error "Unsupported platform"
	return 0;
#endif
}