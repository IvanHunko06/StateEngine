#pragma once
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_events.h"
#include "EngineCore/ApplicationConfigurations/WindowSettings.hpp"
#include "EngineCore/Threading/SPSCQueue.hpp"

#include <thread>
#include <atomic>

using StateEngine::EngineCore::ApplicationConfigurations::WindowSettings;
using StateEngine::EngineCore::Threading::SPSCQueue;
class NativeWindow {
public: using WindowEventsQueue = SPSCQueue<SDL_Event, 2048>;
private:
	std::atomic<SDL_Window*> window_{ nullptr };
	std::thread messageLoopThread_;
	std::atomic_bool stopMessageLoopThread_{ false };
	WindowEventsQueue* eventsQueue_;
public:
	NativeWindow(WindowEventsQueue* eventsQueue)  : eventsQueue_(eventsQueue){}
	NativeWindow(const NativeWindow&) = delete;
	NativeWindow& operator=(const NativeWindow&) = delete;

	void createWindow(const WindowSettings& settings);
	void destroy();

	inline SDL_Window* getSdlWindow() noexcept {
		return window_.load(std::memory_order_relaxed);
	}
private:
	void windowMessageLoop(const WindowSettings& settings);

};