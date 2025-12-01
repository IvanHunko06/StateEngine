#pragma once
#include "TypeRegistryExports.hpp"
#include "TypeInfo.hpp"
#include "TypeInfoCppBuilder.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"
#include <string>

using StateEngine::EngineCore::Hashing::Fnv1aHashProvider;

namespace StateEngine::EngineCore::EngineTypeSystem {
	class TypeRegistryCppLayer {
	public:
		inline static const TypeInfo* getType(size_t hashCode) {
			return TypeRegistry_GetTypeInfo(hashCode);
		}
		template<typename T>
		inline static TypeInfoCppBuilder<T> registerType(size_t hashCode, const char* name, TypeKind kind) {
			return TypeInfoCppBuilder<T>(hashCode, name, kind);
		}

		inline static void unregisterType(size_t hashCode) {
			TypeRegistry_RemoveType(hashCode);
		}
	};
}