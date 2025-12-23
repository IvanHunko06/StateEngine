#pragma once
#include "CompileTimeTypeMeta.hpp"
#include "GetCompileTimeNames.hpp"
#include "TypeInfo.hpp"
#include "TypeRegistryExports.hpp"
#include <type_traits>

namespace StateEngine::EngineCore::EngineTypeSystem {
    class TypeRegistry {
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
            info.ToString        = meta.ToString;

            for (int i = 0; i < meta.FieldCount; ++i) {
                const auto& field = meta.Fields[i];
                FieldInfo fieldInfo;
                fieldInfo.Name         = field.Name;
                fieldInfo.Type         = nullptr;
                fieldInfo.TypeHashCode = field.TypeHash;
                fieldInfo.Offset       = field.Offset;
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
            requires(!std::is_reference_v<T> && !std::is_pointer_v<T> && !std::is_const_v<T>)
        static inline const TypeInfo& GetRequiredType()
        {
            constexpr std::string_view kTypeName = GetTypeName<T>();
            constexpr size_t kTypeHash           = Hashing::Fnv1aHashProvider::HashString(kTypeName);

            return TypeRegistry_GetRequiredType(kTypeHash);
        }

        template <typename T>
            requires(!std::is_reference_v<T> && !std::is_pointer_v<T> && !std::is_const_v<T>)
        static inline const TypeInfo* TryGetType()
        {
            constexpr std::string_view kTypeName = GetTypeName<T>();
            constexpr size_t kTypeHash           = Hashing::Fnv1aHashProvider::HashString(kTypeName);

            return TypeRegistry_TryGetType(kTypeHash);
        }

        template <typename T>
            requires(!std::is_reference_v<T> && !std::is_pointer_v<T> && !std::is_const_v<T>)
        static inline void UnregisterType()
        {
            constexpr std::string_view kTypeName = GetTypeName<T>();
            constexpr size_t kTypeHash           = Hashing::Fnv1aHashProvider::HashString(kTypeName);

            TypeRegistry_RemoveType(kTypeHash);
        }
    };
}  // namespace StateEngine::EngineCore::EngineTypeSystem
