#pragma once
#include "EngineBuilder.hpp"

namespace StateEngine::EngineCore::ApplicationConfigurations {

	class IApplication {
	public:
		virtual ~IApplication() = default;

		virtual void configureEngine(EngineBuilder& builder) = 0;
		virtual void onShutdown() = 0;
	};
}