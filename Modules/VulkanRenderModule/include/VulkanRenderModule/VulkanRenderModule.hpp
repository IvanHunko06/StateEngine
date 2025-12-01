#pragma once
#include "EngineCore/IModule.hpp"
#include "ZVulkanInfo.hpp"
#include "ZVulkanRenderDevice.hpp"

using StateEngine::EngineCore::IModule;
namespace StateEngine::VulkanRenderModule {
	class VulkanRenderModule : public IModule {
	private:
		ZVulkanInfo vulkanInfo_;
		ZVulkanRenderDevice vulkanRenderDevice_;
	public:
		void OnLoad();
		void OnUnload();
		void RegisterTypes();
		void UnregisterTypes();
		inline const char* GetName() const {
			return "VulkanRenderModule";
		}
	};
}