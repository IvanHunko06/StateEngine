#pragma once
#include <cassert>
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"
namespace StateEngine::EngineCore::SmartPointers {

	template <typename T>
	class ZUniquePointer {
	private:
		T* ptr_{ nullptr };
		ZUniquePointer(T* ptr) : ptr_(ptr){}
	public:
		ZUniquePointer() = default;
		ZUniquePointer(const ZUniquePointer&) = delete;
		ZUniquePointer(ZUniquePointer&& other) noexcept: ptr_(other.ptr_) {
			other.ptr_ = nullptr;
		}
		~ZUniquePointer() {
			if (ptr_) {
				ptr_->~T();
				MemoryAllocator_Deallocate(ptr_);
			}

		}

		ZUniquePointer& operator=(const ZUniquePointer&) = delete;
		ZUniquePointer& operator=(ZUniquePointer&& other) noexcept{
			if (this != &other) {
				if (ptr_) {
					ptr_->~T();
					MemoryAllocator_Deallocate(ptr_);
				}
				ptr_ = other.ptr_;
				other.ptr_ = nullptr;
			}
			return *this;
		}

		inline T& operator*() const noexcept {
			assert(ptr_ != nullptr && "Attempted to dereference a null ZUniquePointer!");
			return *ptr_;
		}
		inline T* operator->() const noexcept{
			assert(ptr_ != nullptr && "Attempted to dereference a null ZUniquePointer!");
			return ptr_;
		}
		explicit operator bool() const noexcept {
			return ptr_ != nullptr;
		}

		inline bool operator==(const ZUniquePointer& other) const noexcept{
			return ptr_ == other.ptr_;
		}
		inline bool operator!=(const ZUniquePointer& other) const noexcept{
			return ptr_ != other.ptr_;
		}

		inline T* get() {
			return ptr_;
		}
		inline const T* get() const {
			return ptr_;
		}

		template<typename... Args>
		constexpr static ZUniquePointer make(Args... args) {
			void* data = MemoryAllocator_AlignedAllocate(sizeof(T), alignof(T));
			T* ptr = new (data) T(std::forward<Args>(args)...);
			return ZUniquePointer(ptr);
		}
		constexpr static ZUniquePointer makeFromPointer(T* ptr) {
			return ZUniquePointer(ptr);
		}
	};
}