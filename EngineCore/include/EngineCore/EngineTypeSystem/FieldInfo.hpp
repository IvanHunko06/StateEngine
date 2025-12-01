#pragma once
#include "EngineCore/DataStructures/ZString.hpp"
#include "EngineCore/DataStructures/ZBuffer.hpp"

using namespace StateEngine::EngineCore::DataStructures;

namespace StateEngine::EngineCore::EngineTypeSystem {
	struct TypeInfo;
	struct TypeInstance;

	struct FieldInfo {
		ZString name;
		const TypeInfo* type;
		size_t offset;

		//using GetValueFunction = TypeInstance(*)(TypeInstance* obj);
		//GetValueFunction getValue = nullptr;

		//using SetValueFunction = void(*)(TypeInstance* obj, TypeInstance* value);
		//SetValueFunction setValue = nullptr;
	};
}