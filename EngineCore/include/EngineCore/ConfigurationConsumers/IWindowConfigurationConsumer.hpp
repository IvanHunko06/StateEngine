#pragma once
#include "EngineCore/ApplicationConfigurations/WindowSettings.hpp"
using StateEngine::EngineCore::ApplicationConfigurations::WindowSettings;
namespace StateEngine::EngineCore::ApplicationConfigurations::Consumers {
	class IWindowConfigurationConsumer {
	public:
		virtual void configureWindow(const WindowSettings& settings) = 0;
	};
}