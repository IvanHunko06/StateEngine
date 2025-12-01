#pragma once
#include "EngineCore/DataStructures/ZString.hpp"

using StateEngine::EngineCore::DataStructures::ZString;
namespace StateEngine::EngineCore::ApplicationConfigurations {
	struct WindowSettings {
		ZString title = "StateEngine";
		int width = 640;
		int height = 480;
		enum class DisplayType {
			Windowed,
			BorderlassWindow,
			Fullscreen
		} displayType = WindowSettings::DisplayType::Windowed;
		enum class GraphicAPIHint {
			None,
			OpenGL,
			DirectX,
			Vulkan
		} graphicAPIHint = WindowSettings::GraphicAPIHint::None;
		bool resizable = true;
	};
}