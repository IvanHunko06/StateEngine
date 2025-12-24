#pragma once
#include "CompileTimeTypeMeta.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#include "GetCompileTimeNames.hpp"
#include "HasToStringFunction.hpp"
#include "TypeRegistryExports.hpp"
#include <array>
#include <cstdint>
#include <format>
#include <utility>

namespace StateEngine::EngineCore::EngineTypeSystem {
    template <typename TType, auto... Fields>
    struct CompileTimeTypeBuilder {};

    template <typename TType, auto... Items>
        requires std::is_enum_v<TType>
    struct CompileTimeTypeBuilder<TType, Items...> {
      public:
        using Type = TType;

        static constexpr size_t Count = sizeof...(Items);

        template <int32_t NewVal>
        using AddRaw = CompileTimeTypeBuilder<TType, Items..., static_cast<TType>(NewVal)>;

      private:
        template <typename TBuilder, int32_t Current, int32_t Max>
        struct EnumRangeScanner {
            using TEnum = typename TBuilder::Type;

            static constexpr std::string_view Name = GetEnumItemName<static_cast<TEnum>(Current)>();
            static constexpr bool IsValid          = !Name.empty();

            using NextBuilderState = std::conditional_t<IsValid, typename TBuilder::template AddRaw<Current>, TBuilder>;

            using type =
                typename std::conditional_t<(Current < Max), EnumRangeScanner<NextBuilderState, Current + 1, Max>,
                                            std::type_identity<NextBuilderState>>::type;
        };

      private:
        static DataStructures::ZString ToStringImpl(const void* obj, const char* format)
        {
            constexpr size_t hashCode = Fnv1aHashProvider::HashString(GetTypeName<TType>());
            auto* type                = TypeRegistry_GetType(hashCode);
            if (type == nullptr) {
                return "Unknown";
            }
            const TType value = *static_cast<const TType*>(obj);
            for (auto& enumItem : type->EnumMembers) {
                if (value == static_cast<TType>(enumItem.NumberValue))
                    return enumItem.StringValue;
            }
            return "Unknown";
        }

      public:
        template <int32_t Val>
        using EnumValue = AddRaw<Val>;

        template <int32_t Min = -64, int32_t Max = 64>
        using AutoEnum = typename EnumRangeScanner<CompileTimeTypeBuilder<TType, Items...>, Min, Max>::type;

        static consteval auto Build() noexcept
        {
            using MetaType = CompileTimeTypeMeta<1, Count>;

            MetaType meta {};
            meta.TypeName  = GetTypeName<TType>();
            meta.TypeHash  = Fnv1aHashProvider::HashString(meta.TypeName);
            meta.Size      = sizeof(TType);
            meta.Alignment = alignof(TType);
            meta.EnumCount = Count;
            meta.ToString  = ToStringImpl;

            meta.EnumItems = []() {
                std::array<CompileTimeEnumItemDesc, Count> arr {};
                size_t idx = 0;

                ((arr[idx++] =
                      CompileTimeEnumItemDesc {.Name = GetEnumItemName<Items>(), .Value = static_cast<int32_t>(Items)}),
                 ...);

                return arr;
            }();

            return meta;
        }
    };

    template <typename TType, auto... Fields>
        requires std::is_class_v<TType>
    struct CompileTimeTypeBuilder<TType, Fields...> {
      private:
        using Type                    = TType;
        static constexpr size_t Count = sizeof...(Fields);

        template <auto Ptr>
        struct MemberTraits;

        template <typename T, typename ClassType, T ClassType::* Ptr>
        struct MemberTraits<Ptr> {
            using MemberType = T;
            using HostType   = ClassType;
        };

      private:
        static void CopyCtorImpl(void* dst, const void* src)
        {
            new (dst) TType(*static_cast<const TType*>(src));
        }
        static void MoveCtorImpl(void* dst, void* src)
        {
            new (dst) TType(std::move(*static_cast<const TType*>(src)));
        }
        static void DtorImpl(void* obj)
        {
            static_cast<TType*>(obj)->~TType();
        }
        static DataStructures::ZString ToStringImpl(const void* obj, const char* format)
        {
            const TType* objPtr = reinterpret_cast<const TType*>(obj);
            if constexpr (HasToStringFunction<TType>)
                return objPtr->ToString(format);
            else
                return GetTypeName<TType>().data();
        }
        template <auto Ptr>
        static consteval auto CreateFieldDesc()
        {
            using Traits  = MemberTraits<Ptr>;
            using MemberT = typename Traits::MemberType;

            constexpr std::string_view kName     = GetFieldName<Ptr>();
            constexpr std::string_view kTypeName = GetTypeName<MemberT>();

            return CompileTimeFieldDesc {.Name            = kName,
                                         .Offset          = 0,
                                         .Size            = sizeof(MemberT),
                                         .TypeHash        = Fnv1aHashProvider::HashString(kTypeName),
                                         .FieldHash       = Fnv1aHashProvider::HashString(kName),
                                         .CalculateOffset = []() -> size_t { return Computeoffset(Ptr); }};
        }
        template <typename T, typename TClass>
        static auto Computeoffset(T TClass::* memberptr) noexcept
        {
            const auto fakebase = reinterpret_cast<const TClass*>(0x1000);  // 0x1000 to avoid null check

            const char* memberaddr = reinterpret_cast<const char*>(&(fakebase->*memberptr));

            return reinterpret_cast<size_t>(memberaddr) - 0x1000;
        }

      public:
        using MetaType = CompileTimeTypeMeta<Count, 0>;

      private:
        static consteval auto CompileTimeBuild()
        {
            MetaType meta {};

            meta.TypeName   = GetTypeName<TType>();
            meta.TypeHash   = Fnv1aHashProvider::HashString(meta.TypeName);
            meta.Size       = sizeof(TType);
            meta.Alignment  = alignof(TType);
            meta.FieldCount = Count;
            if constexpr (std::is_copy_constructible_v<TType>) {
                meta.CopyConstructor = CopyCtorImpl;
            }
            if constexpr (std::is_move_constructible_v<TType>) {
                meta.MoveConstructor = MoveCtorImpl;
            }
            if constexpr (std::is_destructible_v<TType>) {
                meta.Destructor = DtorImpl;
            }
            meta.ToString = ToStringImpl;

            meta.Fields = []() {
                std::array<CompileTimeFieldDesc, Count> arr {};
                size_t idx = 0;
                ((arr[idx++] = CreateFieldDesc<Fields>()), ...);
                return arr;
            }();

            return meta;
        }

      public:
        template <auto MemberPtr>
        using Field = CompileTimeTypeBuilder<TType, Fields..., MemberPtr>;

        static auto Build() noexcept
        {
            MetaType runtimeData = CompileTimeBuild();

            for (auto& field : runtimeData.Fields) {
                field.Offset = field.CalculateOffset();
            }

            return std::move(runtimeData);
        }
    };

    template <typename TType>
        requires std::is_fundamental_v<TType>
    struct CompileTimeTypeBuilder<TType> {
        static DataStructures::ZString ToStringImpl(const void* obj, const char* format)
        {
            if constexpr (std::is_void_v<TType>) {
                return "void";
            }
            else {
                if (!obj)
                    return "null";

                const TType value = *static_cast<const TType*>(obj);

                if (format == nullptr || *format == '\0') {
                    if constexpr (std::is_same_v<TType, bool>) {
                        return value ? "true" : "false";
                    }
                    else if constexpr (std::is_same_v<TType, char>) {
                        std::array<char, 2> buf {value, '\0'};
                        return buf.data();
                    }
                    else {
                        return std::format("{}", value).c_str();
                    }
                }

                std::string pattern;
                pattern.reserve(std::char_traits<char>::length(format) + 3);
                pattern.push_back('{');
                pattern.push_back(':');
                pattern.append(format);
                pattern.push_back('}');

                return std::vformat(pattern, std::make_format_args(value)).c_str();
            }
        }

      public:
        static consteval auto Build() noexcept
        {
            using MetaType = CompileTimeTypeMeta<0, 0>;
            MetaType meta;
            meta.TypeName = GetTypeName<TType>();
            meta.TypeHash = Fnv1aHashProvider::HashString(meta.TypeName);
            if constexpr (std::is_void_v<TType>) {
                meta.Size      = 0;
                meta.Alignment = 0;
            }
            else {
                meta.Size      = sizeof(TType);
                meta.Alignment = alignof(TType);
            }

            meta.ToString        = ToStringImpl;
            meta.MoveConstructor = nullptr;
            meta.Destructor      = nullptr;
            meta.CopyConstructor = nullptr;

            return meta;
        }
    };

}  // namespace StateEngine::EngineCore::EngineTypeSystem
