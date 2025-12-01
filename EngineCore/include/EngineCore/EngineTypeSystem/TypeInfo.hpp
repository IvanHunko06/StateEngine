#pragma once
#include "TypeKind.hpp"
#include "MethodInfo.hpp"
#include "FieldInfo.hpp"
#include "EnumMember.hpp"
#include "EngineCore/DataStructures/ZString.hpp"
#include <cstdint>

using StateEngine::EngineCore::DataStructures::ZString;
namespace StateEngine::EngineCore::EngineTypeSystem {
	struct TypeInstance;
	struct TypeInfo {
		ZString name{};
		size_t hashCode{ 0 };
		size_t size{ 1 };
		uint32_t alignment{ 1 };
		TypeKind kind{ TypeKind::Primitive };

		using ToStringFunction = ZString(*)(const TypeInstance& instance, const char* format);
		ToStringFunction toString{ nullptr };

		using MoveConstructorFunction = void(*)(void* dstMemory, void* src);
		MoveConstructorFunction moveConstructor{ nullptr };

		using CopyConstructorFunction = void(*)(void* dstMemory, const void* src);
		CopyConstructorFunction copyConstructor{ nullptr };

		using DestructorFunction = void(*)(void* obj);
		DestructorFunction destructor{ nullptr };


		//ZBuffer<MethodInfo> methods;
		FieldInfo* fieldsPtr{ nullptr };
		uint32_t fieldsCount{ 0 };

		uint32_t enumMembersCount{ 0 };
		EnumMember* enumMembersPtr{ nullptr };
		
		TypeInfo* arrayItem{ nullptr };

		
		TypeInfo() = default;
		TypeInfo(const TypeInfo& other) = delete;
		TypeInfo& operator=(const TypeInfo& other) = delete;

		TypeInfo(TypeInfo&& other) noexcept
			:name(std::move(other.name)),
			hashCode(other.hashCode),
			size(other.size),
			alignment(other.alignment),
			kind(other.kind),
			toString(other.toString),
			moveConstructor(other.moveConstructor),
			copyConstructor(other.copyConstructor),
			destructor(other.destructor),
			fieldsPtr(other.fieldsPtr),
			fieldsCount(other.fieldsCount),
			enumMembersCount(other.enumMembersCount),
			enumMembersPtr(other.enumMembersPtr),
			arrayItem(other.arrayItem)
		{
			other.fieldsPtr = nullptr;
			other.fieldsCount = 0;
			other.enumMembersPtr = nullptr;
			other.enumMembersCount = 0;
			other.arrayItem = nullptr;
		}
		TypeInfo& operator=(TypeInfo&& other) noexcept {
			if (this == &other) return *this;
			name = std::move(other.name);
			hashCode = other.hashCode;
			size = other.size;
			alignment = other.alignment;
			kind = other.kind;
			toString = other.toString;
			moveConstructor = other.moveConstructor;
			copyConstructor = other.copyConstructor;
			destructor = other.destructor;
			fieldsPtr = other.fieldsPtr;
			fieldsCount = other.fieldsCount;
			enumMembersCount = other.enumMembersCount;
			enumMembersPtr = other.enumMembersPtr;
			arrayItem = other.arrayItem;
			other.fieldsPtr = nullptr;
			other.fieldsCount = 0;
			other.enumMembersPtr = nullptr;
			other.enumMembersCount = 0;
			other.arrayItem = nullptr;
			return *this;
		}

		~TypeInfo() {
			if (fieldsPtr) {
				MemoryAllocator_Deallocate(fieldsPtr);
				fieldsPtr = nullptr;
				fieldsCount = 0;
			}
			if (enumMembersPtr) {
				MemoryAllocator_Deallocate(enumMembersPtr);
				enumMembersPtr = nullptr;
				enumMembersCount = 0;
			}
		}
	};
}