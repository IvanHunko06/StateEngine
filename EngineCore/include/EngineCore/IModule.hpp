#pragma once

namespace StateEngine::EngineCore {
	class IModule {
	public:
		virtual ~IModule() = default;
		virtual void OnLoad() = 0;
		virtual void OnUnload() = 0;
		virtual void RegisterTypes() = 0;
		virtual void UnregisterTypes() = 0;
		virtual const char* GetName() const = 0;
	};
}