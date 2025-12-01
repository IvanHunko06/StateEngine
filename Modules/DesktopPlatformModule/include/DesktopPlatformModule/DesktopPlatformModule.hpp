#pragma once
#include "EngineCore/IModule.hpp"
#include "NativeWindow.hpp"
#include "InputEventsSystem.hpp"
#include "EngineCore/ConfigurationConsumers/IWindowConfigurationConsumer.hpp"

using StateEngine::EngineCore::IModule;
using StateEngine::EngineCore::ApplicationConfigurations::Consumers::IWindowConfigurationConsumer;

namespace StateEngine::DesktopPlatformModule {
	class DesktopPlatformModule : 
		public IModule,
		public IWindowConfigurationConsumer
	{
	private:
		NativeWindow gameWindow;
		InputEventsSystem inputEventsSystem_;

	public:
		DesktopPlatformModule(DesktopPlatformModule&&) = delete;
		DesktopPlatformModule(const DesktopPlatformModule&) = delete;
	
	private:
		DesktopPlatformModule() : gameWindow(&inputEventsSystem_.gameWindowEventsQueue){}
	public:
		void OnLoad();
		void OnUnload();
		void RegisterTypes();
		void UnregisterTypes();
		const char* GetName() const {
			return "DesktopPlatformModule";
			
		}
		void configureWindow(const WindowSettings& settings);

	public:
		static DesktopPlatformModule& getInstance();
		inline NativeWindow& getGameWindow() noexcept {
			return gameWindow;
		}
	};
}