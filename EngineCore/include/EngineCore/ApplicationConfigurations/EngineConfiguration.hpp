#pragma once
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/DataStructures/ZString.hpp"
#include "EngineCore/ApplicationConfigurations/WindowSettings.hpp"

using StateEngine::EngineCore::DataStructures::ZBuffer;
using StateEngine::EngineCore::DataStructures::ZString;

namespace StateEngine::EngineCore::ApplicationConfigurations {
	struct EngineConfiguration {
		ZBuffer<ZString> modulesToLoad{};
		WindowSettings windowSettings{};

		using PFN_DependencyRegistrationCallback = void(*)(void* userData);
		PFN_DependencyRegistrationCallback dependencyRegistrationCallback{ nullptr };
		void* dependencyRegistrationCallbackUserData{ nullptr };
#ifdef _WIN32
		bool useCpuSets{ false };
#endif
	};
}