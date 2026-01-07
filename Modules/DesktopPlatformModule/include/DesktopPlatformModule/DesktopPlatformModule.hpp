#pragma once
#include "EngineCore/IModule.hpp"
#include "InputEventsSystem.hpp"
#include "NativeWindow.hpp"

namespace StateEngine::DesktopPlatformModule {
    class DesktopPlatformModule : public StateEngine::EngineCore::IModule {
      private:
        NativeWindow gameWindow;
        InputEventsSystem inputEventsSystem_;

      public:
        DesktopPlatformModule(DesktopPlatformModule&&)      = delete;
        DesktopPlatformModule(const DesktopPlatformModule&) = delete;

      private:
        DesktopPlatformModule() : gameWindow(&inputEventsSystem_.gameWindowEventsQueue) {}

      public:
        void OnLoad();
        void OnUnload();
        void RegisterTypes();
        void UnregisterTypes();
        const char* GetName() const
        {
            return "DesktopPlatformModule";
        }

      public:
        static DesktopPlatformModule& GetInstance();
        inline NativeWindow& GetGameWindow() noexcept
        {
            return gameWindow;
        }
    };
}  // namespace StateEngine::DesktopPlatformModule