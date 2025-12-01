#pragma once
#include "WindowSettings.hpp"
#include "EngineCore/DataStructures/ZString.hpp"
#include "EngineConfiguration.hpp"

using StateEngine::EngineCore::DataStructures::ZString;
namespace StateEngine::EngineCore::ApplicationConfigurations {
	class EngineBuilder {
	public:
		EngineBuilder& WithWindowSettings(const WindowSettings& settings) {
			configuration_.windowSettings = settings;
			return *this;
		}
		EngineBuilder& AddModule(const ZString& path) {
			configuration_.modulesToLoad.push_back(path);
			return *this;
		}
		EngineBuilder& WithDependencyRegistrationCallback(EngineConfiguration::PFN_DependencyRegistrationCallback callback, void* userData) {
			configuration_.dependencyRegistrationCallback = callback;
			configuration_.dependencyRegistrationCallbackUserData = userData;
			return *this;
		}
#ifdef _WIN32
		EngineBuilder& WithCpuSetsUsage(bool usage) {
			configuration_.useCpuSets = usage;
			return *this;
		}
#endif

		EngineConfiguration build() {
			return std::move(configuration_);
		}
	private:
		EngineConfiguration configuration_;
	};
}