#pragma once
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"
#include "TypeInfo.hpp"

namespace StateEngine::EngineCore::EngineTypeSystem {
	struct TypeInstance {
	private:
		void* instancePtr_{ nullptr };
		const TypeInfo* typeInfo_{ nullptr };
		bool isAllocatedInstance_{ false };
	public:
		TypeInstance() = default;
		TypeInstance(void* instancePtr, const TypeInfo* typeInfo): instancePtr_(instancePtr), typeInfo_(typeInfo){}
		TypeInstance(TypeInstance&& other) : instancePtr_(other.instancePtr_), typeInfo_(other.typeInfo_), isAllocatedInstance_(other.isAllocatedInstance_){
			other.instancePtr_ = nullptr;
			other.isAllocatedInstance_ = false;
		}
		TypeInstance(const TypeInstance& other) : typeInfo_(other.typeInfo_) {
			instancePtr_ = MemoryAllocator_AlignedAllocate(typeInfo_->size, typeInfo_->alignment);
			if (instancePtr_) isAllocatedInstance_ = true;
			if (typeInfo_->copyConstructor)
				typeInfo_->copyConstructor(instancePtr_, other.instancePtr_);
		}
		TypeInstance& operator=(const TypeInstance& other) noexcept{
			if (this != &other) {
				if (isAllocatedInstance_ && instancePtr_) {
					if (typeInfo_->destructor)
						typeInfo_->destructor(instancePtr_);
					MemoryAllocator_Deallocate(instancePtr_);
				}
				typeInfo_ = other.typeInfo_;
				instancePtr_ = MemoryAllocator_AlignedAllocate(typeInfo_->size, typeInfo_->alignment);
				if (instancePtr_) isAllocatedInstance_ = true;
				if (typeInfo_->copyConstructor)
					typeInfo_->copyConstructor(instancePtr_, other.instancePtr_);
			}
			return *this;
		}
		TypeInstance& operator=(TypeInstance&& other) noexcept {
			if (this != &other) {
				if (isAllocatedInstance_ && instancePtr_) {
					if (typeInfo_->destructor)
						typeInfo_->destructor(instancePtr_);
					MemoryAllocator_Deallocate(instancePtr_);
				}
				typeInfo_ = other.typeInfo_;
				isAllocatedInstance_ = other.isAllocatedInstance_;
				instancePtr_ = other.instancePtr_;
				other.instancePtr_ = nullptr;
			}
			return *this;
		}
		~TypeInstance() {
			if (isAllocatedInstance_ && instancePtr_) {
				if (typeInfo_->destructor)
					typeInfo_->destructor(instancePtr_);
				MemoryAllocator_Deallocate(instancePtr_);
				instancePtr_ = nullptr;
				isAllocatedInstance_ = false;
			}
		}

		inline void* getRawPtr() const noexcept { return instancePtr_; }
		inline const TypeInfo* getTypeInfo() const noexcept { return typeInfo_; }
	};
}