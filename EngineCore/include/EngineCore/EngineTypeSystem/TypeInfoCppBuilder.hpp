#pragma once
#include "TypeInfo.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/DataStructures/ZString.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistryExports.hpp"
#include "EngineCore/EngineTypeSystem/TypeInstance.hpp"

#include <typeinfo>
#include <type_traits>
#include <concepts>

using namespace StateEngine::EngineCore::DataStructures;

namespace StateEngine::EngineCore::EngineTypeSystem {
	class TypeRegistryCppLayer;

	template<typename T>
	requires (!std::is_union_v<T>)
	class TypeInfoCppBuilder {
	private:
		TypeInfo typeInfo_{};
		ZBuffer<FieldInfo> fields_{};
		ZBuffer<EnumMember> enumMembers_{};

	public:
		TypeInfoCppBuilder(size_t hashCode, const char* name, TypeKind kind) {
			typeInfo_.name = name;
			typeInfo_.hashCode = hashCode;
			typeInfo_.size = sizeof(T);
			typeInfo_.alignment = alignof(T);	
			typeInfo_.kind = kind;
			typeInfo_.toString = nullptr;

			if constexpr (std::is_copy_constructible_v<T>) {
				typeInfo_.copyConstructor = [](void* dstMemory, const void* src) {
					new(reinterpret_cast<T*>(dstMemory)) T(*reinterpret_cast<const T*>(src));
				};
			}
			if constexpr (std::is_move_constructible_v<T>) {
				typeInfo_.moveConstructor = [](void* dstMemory, void* src) {
					new(reinterpret_cast<T*>(dstMemory)) T(std::move(*reinterpret_cast<T*>(src)));
				};
			}

			if constexpr (std::is_destructible_v<T>) {
				typeInfo_.destructor = [](void* obj) {
					reinterpret_cast<T*>(obj)->~T();
				};
			}

			if constexpr (std::is_fundamental_v<T>) {
				typeInfo_.toString = [](const TypeInstance& instance, const char* format)->ZString {
					const T* value = reinterpret_cast<const T*>(instance.getRawPtr());
					char buffer[128];

					if (!format) {
						if constexpr (std::is_integral_v<T>) format = "%lld";
						else if constexpr (std::is_floating_point_v<T>) format = "%f";
						else if constexpr (std::is_same_v<T, bool>) format = "%s";
						else format = "?";
					}

					if constexpr (std::is_integral_v<T>) {
						snprintf(buffer, 128, format, static_cast<long long>(*value));
					}
					else if constexpr (std::is_floating_point_v<T>) {
						snprintf(buffer, 128, format, static_cast<double>(*value));
					}
					else if constexpr (std::is_same_v<T, bool>) {
						snprintf(buffer, 128, format, (*value) ? "true" : "false");
					}
					else {
						snprintf(buffer, 128, "?");
					}

					return ZString(buffer);
				};
			}

		}
		
		template<typename TStruct, typename TField>
		requires (std::is_class_v<T> && !std::is_abstract_v<T> && std::same_as<T, TStruct>)
		void addField(const char* name, TField TStruct::* memberPtr, const TypeInfo* typePtr) {
			FieldInfo fieldInfo;

			T tempObj{};
			
			fieldInfo.name = name;

			size_t offset = (size_t)reinterpret_cast<char*>(&(tempObj.*memberPtr)) -
				(size_t)reinterpret_cast<char*>(&tempObj);

			fieldInfo.offset = offset;
			fieldInfo.type = typePtr;

			fields_.push_back(fieldInfo);
		}


		void addEnumValue(const char* name, T numberValue) {
			EnumMember enumMember;

			enumMember.stringValue = name;
			enumMember.numberValue = static_cast<uint32_t>(numberValue);

			enumMembers_.push_back(enumMember);
		}

		~TypeInfoCppBuilder() {
			if (!fields_.empty()) {
				FieldInfo* heapFields = reinterpret_cast<FieldInfo*>(MemoryAllocator_AlignedAllocate(fields_.size() * sizeof(FieldInfo), alignof(FieldInfo)));
				size_t i = 0;
				for (const auto& field : fields_) {
					new (&heapFields[i++]) FieldInfo(field);
				}
				typeInfo_.fieldsPtr = heapFields;
				typeInfo_.fieldsCount = fields_.size();
			}
			if (!enumMembers_.empty()) {
				EnumMember* heapEnumMembers = reinterpret_cast<EnumMember*>(MemoryAllocator_AlignedAllocate(enumMembers_.size() * sizeof(EnumMember), alignof(EnumMember)));
				size_t i = 0;
				for (const auto& member : enumMembers_) {
					new (&heapEnumMembers[i++]) EnumMember(member);
				}
				typeInfo_.enumMembersPtr = heapEnumMembers;
				typeInfo_.enumMembersCount = enumMembers_.size();
			}
			
			if constexpr (std::is_enum_v<T>) {
				typeInfo_.toString = [](const TypeInstance& instance, const char* format) -> ZString{
					auto numericValue = static_cast<uint32_t>(*reinterpret_cast<const T*>(instance.getRawPtr()));
					const TypeInfo* selfInfo = instance.getTypeInfo();
					if (selfInfo) {
						for (uint32_t i = 0; i < selfInfo->enumMembersCount; ++i) {
							if (selfInfo->enumMembersPtr[i].numberValue == numericValue) {
								return selfInfo->enumMembersPtr[i].stringValue;
							}
						}
					}
					return "<unknown>";
				};
			}
			else if constexpr (std::is_class_v<T>) {
				if (typeInfo_.kind == TypeKind::Struct || typeInfo_.kind == TypeKind::Class) {
					typeInfo_.toString = [](const TypeInstance& instance, const char* format) ->ZString {
						constexpr size_t bufferSize = 4096;

						const TypeInfo* selfInfo = instance.getTypeInfo();
						if (!selfInfo) return "{Error: TypeInfo not found}";

						char buffer[bufferSize];
						int offset = 0;

						offset += snprintf(buffer + offset, bufferSize - offset, "{ ");

						for (uint32_t i = 0; i < selfInfo->fieldsCount; ++i) {
							const FieldInfo* field = &selfInfo->fieldsPtr[i];
							const TypeInfo* fieldType = field->type;
							void* fieldObj = static_cast<char*>(instance.getRawPtr()) + field->offset;

							offset += snprintf(buffer + offset, bufferSize - offset, "%s: ", field->name.c_str());

							ZString fieldValueString;
							if (fieldType && fieldType->toString) {
								fieldValueString = fieldType->toString(TypeInstance(fieldObj, fieldType), nullptr);
							}
							else {
								fieldValueString = "<unknown>";
							}

							offset += snprintf(buffer + offset, bufferSize - offset, "%s, ", fieldValueString.c_str());

							if (offset >= bufferSize - 50) {
								snprintf(buffer + offset, bufferSize - offset, "...");
								break;
							}
						}

						snprintf(buffer + offset, bufferSize - offset, "}");
						return buffer;
						};
				}
			}

			TypeRegistry_RegisterType(std::move(typeInfo_));
		}
	};

	template<>
	class TypeInfoCppBuilder<void> {
	public:
		TypeInfoCppBuilder(size_t hashCode, const char* name, TypeKind kind) {
			TypeInfo typeInfo_{};

			typeInfo_.name = "void";
			typeInfo_.hashCode = hashCode;
			typeInfo_.size = 0;
			typeInfo_.alignment = 0;
			typeInfo_.kind = TypeKind::Primitive;

			typeInfo_.copyConstructor = nullptr;
			typeInfo_.moveConstructor = nullptr;
			typeInfo_.destructor = nullptr;

			typeInfo_.fieldsPtr = nullptr;
			typeInfo_.fieldsCount = 0;
			typeInfo_.enumMembersPtr = nullptr;
			typeInfo_.enumMembersCount = 0;

			TypeRegistry_RegisterType(std::move(typeInfo_));
		}

		~TypeInfoCppBuilder() {}
	};
}