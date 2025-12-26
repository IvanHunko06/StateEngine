#pragma once
#include "EngineCore/ApplicationConfigurations/EngineBuilder.hpp"
#include "EngineCore/ApplicationConfigurations/IApplication.hpp"
#include "EngineCore/EngineUpdatePhase.hpp"
#include "EngineCore/IModule.hpp"
#include "EngineCore/IEngineSystem.hpp"
#include "EventBus/ZEventBus.hpp"
#ifdef _WIN32
#include <Windows.h>
#ifdef ERROR
#undef ERROR
#endif  // ERROR
#endif  // _WIN32

namespace StateEngine::EngineCore {
    class EngineApplication {
      private:
        struct LoadedModuleContext {
            IModule* instance;
#ifdef _WIN32
            HMODULE windowsHandle;
#endif
        };
		static ZBuffer<LoadedModuleContext> LoadedModules;
		static ZBuffer<IEngineSystem*> InputPhaseSystems;
		static ZBuffer<IEngineSystem*> RenderPhaseSystems;
        static EventBus::ZEventBus GlobalEventBus;
      public:
        static void Run(EngineCore::ApplicationConfigurations::IApplication* app);
        static void RegisterEngineSystem(EngineUpdatePhase phase, IEngineSystem* system);
        static EventBus::ZEventBus& GetGlobalEventBus() {
            return GlobalEventBus;
        }

      private:
        static IModule* loadModule(const ZString& path);
        static void applyConfigurations(IModule* module, const ApplicationConfigurations::EngineConfiguration& configuration);
    };
}  // namespace StateEngine::EngineCore