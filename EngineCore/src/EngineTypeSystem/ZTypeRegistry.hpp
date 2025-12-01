#pragma once
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/DataStructures/ZHashMap.hpp"
#include "EngineCore/DataStructures/ZString.hpp"

using StateEngine::EngineCore::DataStructures::ZString;
using StateEngine::EngineCore::DataStructures::ZHashMap;

namespace StateEngine::EngineCore::EngineTypeSystem {
	class ZTypeRegistry {
	private:
		static ZHashMap<size_t, TypeInfo> mapTypes_;
		static bool recordingAllowed_;
		//static std::shared_mutex typesMutex_;

	public:
		static bool RegisterType(TypeInfo&& type);
		static const TypeInfo* GetTypeInfo(size_t hashCode);
		static void RemoveType(size_t hashCode);
		static void RegisterBaseTypes();
		static inline void SetRecordingAllowed(bool newValue) {
			recordingAllowed_ = newValue;
		}
	};
}