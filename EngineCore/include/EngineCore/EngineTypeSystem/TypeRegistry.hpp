#pragma once
#include "CompileTimeTypeMeta.hpp"
#include "GetCompileTimeNames.hpp"
#include "TypeInfo.hpp"
#include "TypeRegistryExports.hpp"
#include <cassert>
#include <type_traits>
#include <string>

namespace StateEngine::EngineCore::EngineTypeSystem {
    template <typename T>
    concept TypeRegistryRestrictionConcept =
        requires { !std::is_reference_v<T> && !std::is_pointer_v<T> && !std::is_const_v<T>; };

    class TypeRegistry {
      private:
        static DataStructures::ZString ToStringStructImpl(TypeKey hashCode, const void* data, const char* format)
        {
            const TypeInfo* type = TryGetType(hashCode);
            if (type == nullptr) {
                return "Unknown";
            }

            std::string buffer;
            buffer.reserve(256);

            buffer += "{";
            bool isFirstField = true;

            for (auto& field : type->Fields) {
                if (!isFirstField) {
                    buffer += ", ";
                }
                isFirstField = false;
                buffer += field.Name.c_str();
                buffer += ": ";

                const void* fieldDataPtr = static_cast<const char*>(data) + field.Offset;
                const TypeInfo* fieldTypeInfo = field.Type;
                if (fieldTypeInfo && fieldTypeInfo->ToString) {
                    DataStructures::ZString val = fieldTypeInfo->ToString(fieldDataPtr, format);
                    buffer += val.c_str();
                }
                else {
                    buffer += "?"; 
                }
            }
            buffer += "}";
            return buffer.c_str();
        }

      public:
        template <size_t N, size_t M>
        static void RegisterType(const CompileTimeTypeMeta<N, M>& meta, TypeKind kind)
        {
            TypeInfo info;

            info.Name            = meta.TypeName;
            info.HashCode        = meta.TypeHash;
            info.Size            = meta.Size;
            info.Alignment       = meta.Alignment;
            info.kind            = kind;
            info.CopyConstructor = meta.CopyConstructor;
            info.MoveConstructor = meta.MoveConstructor;
            info.Destructor      = meta.Destructor;
            if (kind == TypeKind::Struct) {
                info.ToString = [typeHash = meta.TypeHash](const void* data,
                                                           const char* format) -> DataStructures::ZString {
                    return ToStringStructImpl(typeHash, data, format);
                };
            }
            else {
                info.ToString = meta.ToString;
            }

            for (int i = 0; i < meta.FieldCount; ++i) {
                const auto& field = meta.Fields[i];
                FieldInfo fieldInfo;
                fieldInfo.Name         = field.Name;
                fieldInfo.Type         = nullptr;
                fieldInfo.TypeHashCode = field.TypeHash;
                fieldInfo.Offset       = field.Offset;
                fieldInfo.Type         = &GetRequiredType(field.TypeHash);
                info.Fields.push_back(std::move(fieldInfo));
            }

            for (int i = 0; i < meta.EnumCount; ++i) {
                const auto& enumItem = meta.EnumItems[i];
                EnumMember enumMember;
                enumMember.StringValue = enumItem.Name;
                enumMember.NumberValue = enumItem.Value;
                info.EnumMembers.push_back(std::move(enumMember));
            }

            TypeRegistry_RegisterType(std::move(info));
        }

        template <typename T>
            requires TypeRegistryRestrictionConcept<T>
        static inline const TypeInfo& GetRequiredType()
        {
            constexpr std::string_view kTypeName = GetTypeName<T>();
            constexpr size_t kTypeHash           = Hashing::Fnv1aHashProvider::HashString(kTypeName);
            return GetRequiredType(kTypeHash);
        }
        static inline const TypeInfo& GetRequiredType(TypeKey hashCode)
        {
            const TypeInfo* typeInfo = TypeRegistry_GetType(hashCode);
            if (typeInfo == nullptr) {
                assert(false && "TypeRegistry does not have the requested type. The program will terminate.");
                std::terminate();
            }
            return *typeInfo;
        }

        template <typename T>
            requires TypeRegistryRestrictionConcept<T>
        static inline const TypeInfo* TryGetType()
        {
            constexpr std::string_view kTypeName = GetTypeName<T>();
            constexpr size_t kTypeHash           = Hashing::Fnv1aHashProvider::HashString(kTypeName);

            return TypeRegistry_GetType(kTypeHash);
        }
        static inline const TypeInfo* TryGetType(TypeKey hashCode)
        {
            return TypeRegistry_GetType(hashCode);
        }

        template <typename T>
            requires TypeRegistryRestrictionConcept<T>
        static inline void UnregisterType()
        {
            constexpr std::string_view kTypeName = GetTypeName<T>();
            constexpr size_t kTypeHash           = Hashing::Fnv1aHashProvider::HashString(kTypeName);

            TypeRegistry_RemoveType(kTypeHash);
        }
    };
}  // namespace StateEngine::EngineCore::EngineTypeSystem
