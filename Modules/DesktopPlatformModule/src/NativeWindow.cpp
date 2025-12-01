#include "NativeWindow.hpp"
#include "SDL3/SDL_log.h"
#ifdef _WIN32
#include <Windows.h>
#include <sstream>
#endif


void NativeWindow::createWindow(const WindowSettings& settings) {
	if (window_.load(std::memory_order_relaxed)) return;
	messageLoopThread_ = std::thread(&NativeWindow::windowMessageLoop, this, settings);
}
void NativeWindow::destroy() {
	stopMessageLoopThread_.store(true, std::memory_order_relaxed);
	if (messageLoopThread_.joinable()) {
		messageLoopThread_.join();
	}
}

void NativeWindow::windowMessageLoop(const WindowSettings& settings) {
#ifdef _WIN32
	HANDLE hThread = GetCurrentThread();
	std::wstringstream ss;
	ss << settings.title.c_str() << " window message loop thread";
	SetThreadDescription(hThread, ss.str().c_str());
#endif

	SDL_WindowFlags flags = 0;
	if (settings.displayType == WindowSettings::DisplayType::BorderlassWindow)
		flags |= SDL_WINDOW_BORDERLESS;
	else if (settings.displayType == WindowSettings::DisplayType::Fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;

	if (settings.resizable)
		flags |= SDL_WINDOW_RESIZABLE;

	if (settings.graphicAPIHint == WindowSettings::GraphicAPIHint::Vulkan)
		flags |= SDL_WINDOW_VULKAN;
	else if (settings.graphicAPIHint == WindowSettings::GraphicAPIHint::OpenGL)
		flags |= SDL_WINDOW_OPENGL;

	SDL_Window* sdlWindow = SDL_CreateWindow(settings.title.c_str(), settings.width, settings.height, flags);
	window_.store(sdlWindow, std::memory_order_relaxed);
	if (!sdlWindow) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "failed to create SDL window: %s", SDL_GetError());
		return;
	}
	SDL_LogDebug(SDL_LOG_CATEGORY_RENDER, "created SDL window");

	bool running = true;
	while (running && !stopMessageLoopThread_){
		SDL_Event event;
		while (SDL_WaitEvent(&event)){
			eventsQueue_->push(event);
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
				break;
			}
		}
	}
	SDL_DestroyWindow(sdlWindow);
	window_.store(nullptr, std::memory_order_relaxed);
}