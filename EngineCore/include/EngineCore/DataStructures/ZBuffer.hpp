#pragma once
#include <cstdint>
#include <cassert>
#include <algorithm>
#include "ConvertableToItemConcept.hpp"
#include "EngineCore/MemoryManagment/MemoryAllocatorExports.hpp"
#ifdef USE_STL_STRUCTURES
#include <vector>
#include "StlAllocator.hpp"
#endif

namespace StateEngine::EngineCore::DataStructures {
#ifndef USE_STL_STRUCTURES
	template <typename TItem>
	class ZBuffer{
	private:
		static constexpr size_t kBitsInUintptr_t = sizeof(uintptr_t) * CHAR_BIT;
		static constexpr size_t kInlineBufferBytesCapacity = 2 * sizeof(uintptr_t);
		static constexpr size_t kInlineBufferCapacity = kInlineBufferBytesCapacity / sizeof(TItem);
		union BufferDataStorage {
			struct HeapDataStorage{
				TItem* bufferStart_{ nullptr };
				TItem* bufferCurrent_{ nullptr };
				TItem* bufferEnd_{ nullptr };
			};

			struct InlineDataStorage {
				using InlineStorage = std::aligned_storage_t<kInlineBufferBytesCapacity, alignof(TItem)>;
				InlineStorage inlineBuffer_{};
				uint32_t inlineSize_{ 0 };
			};

			HeapDataStorage heap_;
			InlineDataStorage inline_;

		};
		BufferDataStorage data_{};
		
		bool isInline_{ true };

	private:
		void reallocate(size_t newCapacity) noexcept{
			size_t bytesCapacity = newCapacity * sizeof(TItem);
			if (bytesCapacity <= kInlineBufferBytesCapacity) {
				clear();
				if (data_.heap_.bufferStart_ && !isInline_) {
					MemoryAllocator_Deallocate(data_.heap_.bufferStart_);
				}
				isInline_ = true;
				return;
			}
			TItem* newHeapBuffer = reinterpret_cast<TItem*>(MemoryAllocator_AlignedAllocate(bytesCapacity, alignof(TItem)));
			if (!newHeapBuffer) {
				assert(newHeapBuffer && "ZBuffer::reallocate - allocation failed");
				return;
			}

			moveDataToNewBuffer(newHeapBuffer, newCapacity);
			size_t oldSize = size();

			if (data_.heap_.bufferStart_ && !isInline_) {
				MemoryAllocator_Deallocate(data_.heap_.bufferStart_);
			}
			isInline_ = false;
			data_.heap_.bufferStart_ = newHeapBuffer;
			data_.heap_.bufferCurrent_ = newHeapBuffer + oldSize;
			data_.heap_.bufferEnd_ = newHeapBuffer + newCapacity;
		}
		void moveDataToNewBuffer(TItem* newBuffer, size_t newCapacity) noexcept {
			size_t oldSize = size();
			for (size_t i = 0; i < oldSize; ++i) {
				new (&newBuffer[i]) TItem(std::move(getAt(i)));
				getAt(i).~TItem();
			}
		}
		void reset() noexcept {
			data_.inline_.inlineSize_ = 0;
			isInline_ = true;
		}

	public:
		ZBuffer() noexcept{
			reset();
		}
		ZBuffer(size_t initialSize) noexcept{
			resize(initialSize);
		}
		ZBuffer(ZBuffer&& other) noexcept {
			if (other.isInline_) {
				isInline_ = true;
				data_.inline_.inlineSize_ = other.data_.inline_.inlineSize_;

				TItem* this_front = front();
				TItem* other_front = other.front();

				for (size_t i = 0; i < size(); ++i) {
					new (&this_front[i]) TItem(std::move(other_front[i]));
					other_front[i].~TItem();
				}

				other.data_.inline_.inlineSize_ = 0;

			}
			else {
				isInline_ = false;
				data_.heap_ = other.data_.heap_;

				other.isInline_ = true;
				other.data_.inline_.inlineSize_ = 0;
			}
		}
		ZBuffer(const ZBuffer& other) {
			resize(other.size());
			size_t i = 0;
			for (const auto& item : other) {
				new (&getAt(i++)) TItem(item);
			}
		}
		ZBuffer(std::initializer_list<TItem> initList) noexcept {
			resize(initList.size());
			size_t i = 0;
			for (const auto& item : initList) {
				new (&getAt(i++)) TItem(item);
			}
		}
		~ZBuffer() {
			clear();
			if (!isInline_ && data_.heap_.bufferStart_) {
				MemoryAllocator_Deallocate(data_.heap_.bufferStart_);
			}
			reset();
		}
		ZBuffer& operator=(ZBuffer&& other) noexcept {
			if (this == &other) return *this;

			clear();
			if (!isInline_) {
				MemoryAllocator_Deallocate(data_.heap_.bufferStart_);
			}

			if (other.isInline_) {
				isInline_ = true;
				data_.inline_.inlineSize_ = other.data_.inline_.inlineSize_;

				TItem* this_front = front();
				TItem* other_front = other.front();

				for (size_t i = 0; i < size(); ++i) {
					new (&this_front[i]) TItem(std::move(other_front[i]));
					other_front[i].~TItem();
				}
				other.data_.inline_.inlineSize_ = 0;
			}
			else {
				isInline_ = false;
				data_.heap_ = other.data_.heap_;

				other.isInline_ = true;
				other.data_.inline_.inlineSize_ = 0;
			}
			return *this;
		}
		ZBuffer& operator=(const ZBuffer& other) noexcept{
			if (this != &other) {
				clear();
				if (!isInline_) {
					MemoryAllocator_Deallocate(data_.heap_.bufferStart_);
				}
				resize(other.size());
				size_t i = 0;
				for (const auto& item : other) {
					new (&getAt(i++)) TItem(item);
				}
			}
			return *this;
		}

		// ---------- Buffer functions -------------------------------------------
		inline size_t size() const noexcept {
			return isInline_ ? 
				data_.inline_.inlineSize_ : 
				data_.heap_.bufferCurrent_ - data_.heap_.bufferStart_;
		}
		inline size_t capacity() const noexcept {
			return isInline_ ? 
				kInlineBufferBytesCapacity / sizeof(TItem) : 
				data_.heap_.bufferEnd_ - data_.heap_.bufferStart_;
		}
		inline bool empty() const noexcept {
			return size() == 0;
		}
		inline TItem* front() noexcept{
			return isInline_
				? reinterpret_cast<TItem*>(&data_.inline_.inlineBuffer_)
				: data_.heap_.bufferStart_;
		}
		inline const TItem* front() const noexcept {
			return isInline_
				? reinterpret_cast<const TItem*>(&data_.inline_.inlineBuffer_)
				: data_.heap_.bufferStart_;
		}
		inline TItem* back() noexcept{
			return isInline_
				? reinterpret_cast<TItem*>(&data_.inline_.inlineBuffer_) + data_.inline_.inlineSize_
				: data_.heap_.bufferCurrent_;
		}
		inline const TItem* back() const noexcept {
			return isInline_
				? reinterpret_cast<const TItem*>(&data_.inline_.inlineBuffer_) + data_.inline_.inlineSize_
				: data_.heap_.bufferCurrent_;
		}

		inline void clear() noexcept{
			TItem* dataBegin = front();
			for (size_t i = 0; i < size(); ++i) {
				dataBegin[i].~TItem();
			}
			if (!isInline_)
				data_.heap_.bufferCurrent_ = data_.heap_.bufferStart_;
			else 
				data_.inline_.inlineSize_ = 0;
		}

		template<ConvertibleToItem<TItem> T>
		void push_back(T&& item) noexcept{
			if (size() >= capacity()) {
				size_t bufferCapacity = capacity();
				size_t newCap = (bufferCapacity == 0) ? 4 : bufferCapacity * 2;
				reallocate(newCap);
			}

			if (size() >= capacity()) {
				// reallocation failed
				assert("ZBuffer::pushBack - reallocation failed");
				return;
			}

			new (back()) TItem(std::forward<T>(item));

			if (isInline_)
				++data_.inline_.inlineSize_;
			else
				data_.heap_.bufferCurrent_ += 1;
		}

		template<ConvertibleToItem<TItem> T>
		void insertAt(size_t idx, T&& item) noexcept {

			if (size() >= capacity()) {
				size_t bufferCapacity = capacity();
				size_t newCap = (bufferCapacity == 0) ? 4 : bufferCapacity * 2;
				reallocate(newCap);
			}

			TItem* bufferStart = front();
			size_t currentSize = size();
			for (size_t i = currentSize; i > idx; --i) {
				new(&bufferStart[i]) TItem(std::move(bufferStart[i - 1]));
				bufferStart[i - 1].~TItem();
			}

			new (&bufferStart[idx]) TItem(std::forward<T>(item));

			if (isInline_)
				++data_.inline_.inlineSize_;
			else
				data_.heap_.bufferCurrent_ += 1;
		}

		void removeAt(size_t idx) noexcept{
			size_t bufferSize = size();
			if (idx >= bufferSize) {
				assert("ZBuffer::remove_at - index out of range");
				return;
			}
			getAt(idx).~TItem();
			TItem* bufferBegin = front();
			for (size_t i = idx; i < bufferSize - 1; ++i) {
				new(&bufferBegin[i]) TItem(std::move(bufferBegin[i + 1]));
				bufferBegin[i + 1].~TItem();
			}

			if (isInline_)
				--data_.inline_.inlineSize_;
			else
				data_.heap_.bufferCurrent_ -= 1;

			size_t newSize = bufferSize - 1;
			if (newSize <= kInlineBufferCapacity && !isInline_) {
				reallocate(kInlineBufferCapacity);
			}
		}
		inline void removeLast() noexcept{
			back()[-1].~TItem();
			if (isInline_)
				--data_.inline_.inlineSize_;
			else
				data_.heap_.bufferCurrent_ -= 1;

			if (size() < kInlineBufferCapacity && !isInline_) {
				reallocate(kInlineBufferCapacity);
			}
		}

		inline TItem& getAt(size_t idx) noexcept{
			assert(idx < size() && "ZBuffer::getAt - index out of range");
			return front()[idx];
		}
		inline const TItem& getAt(size_t idx) const noexcept {
			assert(idx < size() && "ZBuffer::getAt - index out of range");
			return front()[idx];
		}
		inline TItem& operator[](size_t idx) noexcept {
			return getAt(idx);
		}

		void resize(size_t newSize, const TItem& defaultValue) noexcept {
			if constexpr (std::copy_constructible<TItem>) {
				size_t curSize = size();
				newSize = (std::max)(curSize, newSize);

				reallocate(newSize);
				for (size_t i = curSize; i < newSize; ++i) {
					new (back() + i) TItem(defaultValue);
				}
				if(isInline_)
					data_.inline_.inlineSize_ = static_cast<uint32_t>(newSize);
				else
					data_.heap_.bufferCurrent_ = data_.heap_.bufferStart_ + newSize;
			}
		}
		void resize(size_t newSize) noexcept {
			size_t curSize = size();
			newSize = (std::max)(curSize, newSize);

			reallocate(newSize);
			for (size_t i = curSize; i < newSize; ++i) {
				new (back() + i) TItem();
			}
			if (isInline_)
				data_.inline_.inlineSize_ = static_cast<uint32_t>(newSize);
			else
				data_.heap_.bufferCurrent_ = data_.heap_.bufferStart_ + newSize;
		}
		inline void reserve(size_t newSize) {
			if (newSize > size())
				reallocate(newSize);
		}

	public:
		// ------------ Iterators -------------------------------------------------

		template<typename TIteratorType>
		struct alignas(std::max_align_t) iterator {
			TIteratorType* ptr;
			size_t index;


			iterator(TIteratorType* ptr, size_t index) : ptr(ptr), index(index){}
			TIteratorType& operator*() const {
				return ptr[index];
			}

			TIteratorType* operator->() const {
				return &ptr[index];
			}

			iterator<TIteratorType>& operator++() { ++index; return *this; }
			iterator<TIteratorType> operator++(int) { iterator<TIteratorType> tmp = *this; ++(*this); return tmp; }

			bool operator==(const iterator<TIteratorType>& other) const { return index == other.index; }
			bool operator!=(const iterator<TIteratorType>& other) const { return index != other.index; }
		};
		
		using standartIterator = iterator<TItem>;
		using constIterator = iterator<const TItem>;

		standartIterator begin() { return standartIterator(front(), 0); }
		standartIterator end() { return standartIterator(front(), size()); }

		constIterator begin() const { return constIterator(front(), 0); }
		constIterator end() const { return constIterator(front(), size()); }
	};
#else
template<typename TItem>
using ZBuffer = std::vector<TItem, StlAllocator<TItem>>;
#endif
}