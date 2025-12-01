#pragma once
#include "EngineCore/DataStructures/ZString.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"

using namespace StateEngine::EngineCore::DataStructures;

namespace StateEngine::EngineCore::EngineTypeSystem {
	struct TypeInfo;
	struct TypeInstance;

	struct MethodInfo {
		ZString name;
		const TypeInfo* returnValueType;
		ZBuffer<const TypeInfo*> args;

		using InvokeFunction = void(*)(TypeInstance* obj, const ZBuffer<TypeInstance*>& args, TypeInstance* returnValue);
		InvokeFunction invoke = nullptr;
	};
}