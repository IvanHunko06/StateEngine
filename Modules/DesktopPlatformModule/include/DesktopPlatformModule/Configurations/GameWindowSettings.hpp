#pragma once
#include "EngineCore/DataStructures/ZString.hpp"

namespace StateEngine::DesktopPlatformModule::Configurations {
    struct GameWindowSettings {
        EngineCore::DataStructures::ZString title = "StateEngine";
        int width                                 = 640;
        int height                                = 480;
        enum class DisplayType {
            Windowed,
            BorderlassWindow,
            Fullscreen
        } displayType                                                     = GameWindowSettings::DisplayType::Windowed;
        enum class GraphicAPIHint { None, OpenGL, Vulkan } graphicAPIHint = GameWindowSettings::GraphicAPIHint::None;
        bool resizable                                                    = true;
    };
}  // namespace StateEngine::DesktopPlatformModule::Configurations