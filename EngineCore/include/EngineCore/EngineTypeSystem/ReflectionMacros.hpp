#pragma once
#include "TypeRegistryCppLayer.hpp"
#include "EngineCore/Hashing/Fnv1aHashProvider.hpp"


#define REGISTER_PRIMITIVE(NAME, TYPE) \
	{ \
		constexpr size_t hash = StateEngine::EngineCore::Hashing::Fnv1aHashProvider::hashString(NAME); \
		StateEngine::EngineCore::EngineTypeSystem::TypeRegistryCppLayer::registerType<TYPE>(hash, NAME, StateEngine::EngineCore::EngineTypeSystem::TypeKind::Primitive);\
	}

#define REGISTER_INTERFACE(NAME, TYPE) \
	{ \
		constexpr size_t hash = StateEngine::EngineCore::Hashing::Fnv1aHashProvider::hashString(NAME); \
		StateEngine::EngineCore::EngineTypeSystem::TypeRegistryCppLayer::registerType<TYPE>(hash, NAME, StateEngine::EngineCore::EngineTypeSystem::TypeKind::Interface);\
	}

#define BEGIN_REFLECT_ENUM(NAME, TYPE) \
	{\
		constexpr size_t hash = StateEngine::EngineCore::Hashing::Fnv1aHashProvider::hashString(NAME); \
		StateEngine::EngineCore::EngineTypeSystem::TypeInfoCppBuilder<TYPE> builder = StateEngine::EngineCore::EngineTypeSystem::TypeRegistryCppLayer::registerType<TYPE>(hash, NAME, StateEngine::EngineCore::EngineTypeSystem::TypeKind::Enum);

#define REFLECT_ENUM_VALUE(NAME, VALUE) \
		builder.addEnumValue(NAME, VALUE);

#define BEGIN_REFLECT_STRUCT(NAME, TYPE) \
	{\
		using _ReflectedType = TYPE; \
		constexpr size_t hash = StateEngine::EngineCore::Hashing::Fnv1aHashProvider::hashString(NAME); \
		StateEngine::EngineCore::EngineTypeSystem::TypeInfoCppBuilder<TYPE> builder = StateEngine::EngineCore::EngineTypeSystem::TypeRegistryCppLayer::registerType<TYPE>(hash, NAME, StateEngine::EngineCore::EngineTypeSystem::TypeKind::Struct);

#define BEGIN_REFLECT_CLASS(NAME, TYPE) \
	{\
		using _ReflectedType = TYPE; \
		constexpr size_t hash = StateEngine::EngineCore::Hashing::Fnv1aHashProvider::hashString(NAME); \
		StateEngine::EngineCore::EngineTypeSystem::TypeInfoCppBuilder<TYPE> builder = StateEngine::EngineCore::EngineTypeSystem::TypeRegistryCppLayer::registerType<TYPE>(hash, NAME, StateEngine::EngineCore::EngineTypeSystem::TypeKind::Class);

#define REFLECT_FIELD(FIELD_NAME, FIELD_TYPE_NAME) \
		builder.addField(#FIELD_NAME, &_ReflectedType::FIELD_NAME, StateEngine::EngineCore::EngineTypeSystem::TypeRegistryCppLayer::getType(StateEngine::EngineCore::Hashing::Fnv1aHashProvider::hashString(FIELD_TYPE_NAME)));

#define END_REFLECT \
	}