#pragma once
#include "ReflectionMacros.hpp"
#include "TypeRegistryCppLayer.hpp"

#define GET_TYPE_INFO(NAME) \
	StateEngine::EngineCore::EngineTypeSystem::TypeRegistryCppLayer::getType(\
	StateEngine::EngineCore::Hashing::Fnv1aHashProvider::hashString(NAME))

#define UNREGISTER_TYPE(NAME) \
	StateEngine::EngineCore::EngineTypeSystem::TypeRegistryCppLayer::unregisterType(\
	StateEngine::EngineCore::Hashing::Fnv1aHashProvider::hashString(NAME))