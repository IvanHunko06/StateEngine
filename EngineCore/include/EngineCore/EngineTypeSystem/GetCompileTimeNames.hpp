#pragma once
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#include <array>
#include <string_view>
#include <type_traits>

namespace StateEngine::EngineCore::EngineTypeSystem {
    template <typename T>
        requires(std::is_class_v<T> || std::is_enum_v<T> || std::is_fundamental_v<T>) &&
                (!std::is_pointer_v<T> && !std::is_reference_v<T>)
    consteval std::string_view GetTypeName()
    {
        std::string_view name;
#if defined(__clang__) || defined(__GNUC__)
        name = __PRETTY_FUNCTION__;
        // Parsing the string "constexpr auto GetTypeName() [with T = MyNamespace::MyType]"
        const size_t prefix = name.find("T = ") + 4;
        const size_t suffix = name.find_last_of(']');
        name                = name.substr(prefix, suffix - prefix);
#elif defined(_MSC_VER)
        name = __FUNCSIG__;
        // Parsing for MSVC
        const size_t prefix = name.find("GetTypeName<") + 12;
        const size_t suffix = name.find_last_of('>');
        name                = name.substr(prefix, suffix - prefix);
        // Remove "struct " or "class " if needed
        if (name.starts_with("struct "))
            name.remove_prefix(7);
        if (name.starts_with("class "))
            name.remove_prefix(6);
        if (name.starts_with("enum "))
            name.remove_prefix(5);
#endif
        return name;
    }

    template <auto V>
        requires std::is_enum_v<decltype(V)>
    consteval std::string_view GetEnumItemName()
    {
#if defined(__clang__) || defined(__GNUC__)
        std::string_view name = __PRETTY_FUNCTION__;
        // Output format: "... [V = Color::Red]"

        auto start = name.find("V = ");
        if (start == std::string_view::npos)
            return {};

        name     = name.substr(start + 4);
        auto end = name.find_last_of(']');
        if (end == std::string_view::npos)
            return {};
        name = name.substr(0, end);

#elif defined(_MSC_VER)
        std::string_view name = __FUNCSIG__;
        // Output format: "...GetEnumItemName<enum Namespace::Color::Red>(void)"
        // or "...GetEnumItemName<(enum Color)123>(void)" if invalid

        auto end = name.find_last_of('>');
        if (end == std::string_view::npos)
            return {};

        auto start = name.find_last_of('<', end);
        if (start == std::string_view::npos)
            return {};

        name = name.substr(start + 1, end - start - 1);
#else
        return {};
#endif

        if (name.empty() || name.front() == '(' || (name.front() >= '0' && name.front() <= '9') ||
            name.front() == '-') {
            return {};
        }

        auto colon = name.find_last_of(':');
        if (colon != std::string_view::npos) {
            name = name.substr(colon + 1);
        }

        return name;
    }

    template <auto Ptr>
        requires std::is_member_object_pointer_v<decltype(Ptr)>
    consteval std::string_view GetFieldName()
    {
#if defined(_MSC_VER)
        std::string_view func = __FUNCSIG__;
        // MSVC Output looks like:
        // "const class std::basic_string_view<...> __cdecl GetFieldName<class MyClass, &MyClass::myField>(void)"
        // or "...GetFieldName<struct MyStruct, &MyStruct::myField>(void)"

        // Look for the beginning of the field name (after the last "::")
        // This is simplified logic, but it works for most cases
        auto end       = func.find_last_of('>');       // End of template argument
        auto lastColon = func.find_last_of(':', end);  // Colon before field name

        // Return the part of the string between :: and >
        return func.substr(lastColon + 1, end - (lastColon + 1));

#elif defined(__clang__) || defined(__GNUC__)
        std::string_view func = __PRETTY_FUNCTION__;
        // Clang Output: "consteval std::string_view Reflection::GetFieldName() [Ptr = &MyClass::myField]"

        auto end       = func.find_last_of(']');
        auto lastColon = func.find_last_of(':', end);

        return func.substr(lastColon + 1, end - (lastColon + 1));
#else
        return "UnknownField";
#endif
    }

}  // namespace StateEngine::EngineCore::EngineTypeSystem