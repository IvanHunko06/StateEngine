#pragma once
#include "EngineCore/DataStructures/ZBuffer.hpp"
#include "EngineCore/DataStructures/ZString.hpp"
#include "EngineCore/DataStructures/ZFunction.hpp"
#include "EngineCore/ApplicationConfigurations/WindowSettings.hpp"

using StateEngine::EngineCore::DataStructures::ZBuffer;
using StateEngine::EngineCore::DataStructures::ZString;
using StateEngine::EngineCore::DataStructures::ZFunction;

namespace StateEngine::EngineCore::ApplicationConfigurations {
	struct EngineConfiguration {
		ZBuffer<ZString> modulesToLoad{};
		WindowSettings windowSettings{};

		ZFunction<void()> dependencyRegistrationCallback;
#ifdef _WIN32
		bool useCpuSets{ false };
#endif
	};
}