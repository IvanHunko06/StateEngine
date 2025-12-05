#pragma once
#include "EngineCore/StringCollection/StringCollectionExports.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"

using StateEngine::EngineCore::Hashing::Fnv1aHashProvider;
namespace StateEngine::EngineCore::DataStructures {
	class ZString {
	private:
		uint32_t stringLength{ 0 };
		const char* stringData{ nullptr };
		size_t hashCache{ 0 };
	public:
		ZString() {
			this->stringData = nullptr;
			this->stringLength = 0;
			this->hashCache = 0;
		}
		ZString(const ZString& other) : 
			stringLength(other.stringLength), 
			stringData(other.stringData),
			hashCache(other.hashCache)
		{
			if (stringData) {
				StringCollection_IncrementRefCount(stringData);
			}
		}
		ZString(const char* str) {
			stringData = StringCollection_GetOrCreateSharedString(str);
			stringLength = 0;
			if (!stringData) return;

			stringLength = static_cast<uint32_t>(strlen(stringData) + 1);
			if (stringLength > 1 && stringData) {
				hashCache = Fnv1aHashProvider::hashBytes(c_str(), length() - 1);
			}
			
		}
		ZString(ZString&& other) noexcept : 
			stringLength(other.stringLength), 
			stringData(other.stringData),
			hashCache(other.hashCache)
		{
			other.stringData = nullptr;
			other.stringLength = 0;
		}
		~ZString() {
			if (stringData) {
				StringCollection_DecrementRefCount(stringData);
			}
			this->stringData = nullptr;
			this->stringLength = 0;
			this->hashCache = 0;
		}

		ZString& operator=(const ZString& other) {
			if (this == &other) return *this;
			if (stringData) {
				StringCollection_DecrementRefCount(stringData);
			}
			stringLength = other.stringLength;
			stringData = other.stringData;
			hashCache = other.hashCache;
			if (stringData) {
				StringCollection_IncrementRefCount(stringData);
			}
			return *this;
		}
		ZString& operator=(ZString&& other) noexcept {
			if (this == &other) return *this;
			if (stringData) {
				StringCollection_DecrementRefCount(stringData);
			}
			stringLength = other.stringLength;
			stringData = other.stringData;
			hashCache = other.hashCache;
			other.stringData = nullptr;
			other.stringLength = 0;
			other.hashCache = 0;
			return *this;
		}

		inline bool empty() const noexcept { return length() == 0 || stringData == nullptr || strcmp(stringData, "") == 0; };
		inline size_t Hash() const noexcept{
			return hashCache;
		}
		inline const char* c_str() const noexcept{ return stringData; }
		inline uint32_t length() const noexcept{
			return stringLength;
		}

		inline bool operator==(const ZString& other) const noexcept{
			if (stringLength != other.stringLength) return false;
			if (stringData == other.stringData) return true;
			if (!stringData || !other.stringData) return false;
			return strcmp(stringData, other.stringData) == 0;
		}
		inline bool operator==(const char* other) const noexcept{
			if (!other) return stringData == nullptr;
			if (!stringData) return false;
			return strcmp(stringData, other) == 0;
		}

		inline bool operator!=(const ZString& other) const noexcept{
			return !(*this == other);
		}
		inline bool operator!=(const char* other) const noexcept{
			return !(*this == other);
		}
	};
}