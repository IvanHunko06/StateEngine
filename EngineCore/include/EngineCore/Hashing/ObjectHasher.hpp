#pragma once
#include "HasHashFunctionConcept.hpp"
#include "HashProviderConcept.hpp"
#include "HashStdHashConcept.hpp"
#include "IsRawHashable.hpp"

namespace StateEngine::EngineCore::Hashing {

    template <HashProvider THashProvider>
    class ObjectHasher {
      public:
        ObjectHasher() = default;

        template <typename T>
        size_t operator()(const T& value) const noexcept
        {
            if constexpr (HasHashFunction<T>) {
                return value.Hash();
            }
            else if constexpr (HasStdHash<T>) {
                return std::hash<T> {}(value);
            }
            else if constexpr (IsRawHashable<T>) {
                return THashProvider::hashBytes(&value, sizeof(value));
            }
            else {
                static_assert(!std::is_same_v<T, T>,
                              "Type is not hashable. To make it hashable, you can:\n"
                              "1. Add a public member function 'size_t Hash() const'.\n"
                              "2. Specialize 'std::hash<T>'.\n"
                              "3. Ensure the type is trivial (e.g., simple struct) to be hashed by its raw bytes.");
            }
        }
    };
}  // namespace StateEngine::EngineCore::Hashing