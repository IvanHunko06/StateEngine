#pragma once

namespace StateEngine::EngineCore::EngineTypeSystem {
	enum class TypeKind {
		Primitive,  // methods = 0, fields = 0, enumMembers = 0
		Struct,		// methods = 0, fields > 0, enumMembers = 0
		Class,		// enumMembers = 0
		Interface,  // fields = 0, enumMembers = 0
		Enum,		// methods = 0, fields = 0, enumMembers > 0,
		Array
	};
}
