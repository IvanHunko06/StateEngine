#pragma once
#include "DesktopPlatformModule/Configurations/GameWindowSettings.hpp"
#include "EngineCore/Threading/SPSCQueue.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"

#include <atomic>
#include <thread>

class NativeWindow {
  public:
    using WindowEventsQueue = StateEngine::EngineCore::Threading::SPSCQueue<SDL_Event, 2048>;

  private:
    std::atomic<SDL_Window*> window_ {nullptr};
    std::thread messageLoopThread_;
    std::atomic_bool stopMessageLoopThread_ {false};
    WindowEventsQueue* eventsQueue_;

  public:
    NativeWindow(WindowEventsQueue* eventsQueue) : eventsQueue_(eventsQueue) {}
    NativeWindow(const NativeWindow&)            = delete;
    NativeWindow& operator=(const NativeWindow&) = delete;

    void CreateWindow(const StateEngine::DesktopPlatformModule::Configurations::GameWindowSettings& settings);
    void Destroy();

    inline SDL_Window* GetSdlWindow() noexcept
    {
        return window_.load(std::memory_order_relaxed);
    }

  private:
    void WindowMessageLoop(const StateEngine::DesktopPlatformModule::Configurations::GameWindowSettings& settings);
};