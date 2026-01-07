#include "DesktopPlatformModule/DesktopPlatformModule.hpp"
#include "DesktopPlatformModule/Configurations/GameWindowSettings.hpp"
#include "EngineCore/ApplicationExports.hpp"
#include "EngineCore/EngineTypeSystem/CompileTimeTypeBuilder.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistry.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/ServiceLocator/ServiceLocator.hpp"
#include "SDL3/SDL.h"

using StateEngine::DesktopPlatformModule::DesktopPlatformModule;
using StateEngine::EngineCore::EngineUpdatePhase;
using namespace StateEngine::EngineCore::EngineTypeSystem;
using namespace StateEngine::DesktopPlatformModule::Configurations;
using namespace StateEngine::EngineCore::ServiceLocator;

namespace {
    static constexpr const char* GetSdlLogCategoryName(SDL_LogCategory category)
    {
        switch (category) {
            case SDL_LOG_CATEGORY_APPLICATION: return "application";
            case SDL_LOG_CATEGORY_ERROR: return "error";
            case SDL_LOG_CATEGORY_ASSERT: return "assert";
            case SDL_LOG_CATEGORY_SYSTEM: return "system";
            case SDL_LOG_CATEGORY_AUDIO: return "audio";
            case SDL_LOG_CATEGORY_VIDEO: return "video";
            case SDL_LOG_CATEGORY_RENDER: return "render";
            case SDL_LOG_CATEGORY_INPUT: return "input";
            case SDL_LOG_CATEGORY_GPU: return "gpu";
            case SDL_LOG_CATEGORY_CUSTOM: return "custom";
            default: return "none";
        }
    }
}  // namespace

void DesktopPlatformModule::OnLoad()
{
    SDL_SetMemoryFunctions([](size_t sz) -> void* { return MemoryAllocator_AlignedAllocate(sz, 16); },
                           [](size_t nmemb, size_t sz) -> void* { return MemoryAllocator_Calloc(nmemb, sz, 16); },
                           [](void* mem, size_t sz) -> void* { return MemoryAllocator_Realloc(mem, sz, 16); },
                           MemoryAllocator_Deallocate);

    SDL_SetLogOutputFunction(
        [](void* userdata, int category, SDL_LogPriority priority, const char* message) {
            switch (priority) {
                case SDL_LOG_PRIORITY_DEBUG:
                    ZLOG_DEBUG("SDL3") << "[category:" << GetSdlLogCategoryName(static_cast<SDL_LogCategory>(category))
                                       << "]: " << message;
                    break;
                case SDL_LOG_PRIORITY_INFO:
                    ZLOG_INFO("SDL3") << "[category:" << GetSdlLogCategoryName(static_cast<SDL_LogCategory>(category))
                                      << "]: " << message;
                    break;
                case SDL_LOG_PRIORITY_WARN:
                    ZLOG_WARN("SDL3") << "[category:" << GetSdlLogCategoryName(static_cast<SDL_LogCategory>(category))
                                      << "]: " << message;
                    break;
                case SDL_LOG_PRIORITY_ERROR:
                    ZLOG_ERROR("SDL3") << "[category:" << GetSdlLogCategoryName(static_cast<SDL_LogCategory>(category))
                                       << "]: " << message;
                    break;
                case SDL_LOG_PRIORITY_CRITICAL:
                    ZLOG_FATAL("SDL3") << "[category:" << GetSdlLogCategoryName(static_cast<SDL_LogCategory>(category))
                                       << "]: " << message;
                    break;
                default: break;
            }
        },
        nullptr);

    LogLevel sdlLogLevel = Logger_GetLogLevel("SDL3");
    switch (sdlLogLevel) {
        case StateEngine::EngineCore::Logging::LogLevel::DEBUG: SDL_SetLogPriorities(SDL_LOG_PRIORITY_DEBUG); break;
        case StateEngine::EngineCore::Logging::LogLevel::INFO: SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO); break;
        case StateEngine::EngineCore::Logging::LogLevel::WARNING: SDL_SetLogPriorities(SDL_LOG_PRIORITY_WARN); break;
        case StateEngine::EngineCore::Logging::LogLevel::ERROR: SDL_SetLogPriorities(SDL_LOG_PRIORITY_ERROR); break;
        case StateEngine::EngineCore::Logging::LogLevel::FATAL: SDL_SetLogPriorities(SDL_LOG_PRIORITY_CRITICAL); break;
        default: break;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL initialization failed: %s", SDL_GetError());
        return;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SDL initialized");
    Application_RegisterSystem(EngineUpdatePhase::Input, &inputEventsSystem_);

    const auto& gameWindowSettings = ServiceLocator::GetRequiredService<GameWindowSettings>();
    gameWindow.CreateWindow(gameWindowSettings);
}
void DesktopPlatformModule::OnUnload()
{
    gameWindow.Destroy();
    SDL_Quit();
}
void DesktopPlatformModule::RegisterTypes()
{
    TypeRegistry::RegisterType(CompileTimeTypeBuilder<GameWindowSettings::DisplayType>::AutoEnum<>::Build(),
                               TypeKind::Enum);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<GameWindowSettings::GraphicAPIHint>::AutoEnum<>::Build(),
                               TypeKind::Enum);

    auto gameWindowSettingsType = CompileTimeTypeBuilder<GameWindowSettings>::Field<&GameWindowSettings::title>::Field<
        &GameWindowSettings::width>::Field<&GameWindowSettings::height>::Field<&GameWindowSettings::displayType>::
        Field<&GameWindowSettings::graphicAPIHint>::Field<&GameWindowSettings::resizable>::Build();
    TypeRegistry::RegisterType(gameWindowSettingsType, TypeKind::Struct);
}
void DesktopPlatformModule::UnregisterTypes()
{
    TypeRegistry::UnregisterType<GameWindowSettings::DisplayType>();
    TypeRegistry::UnregisterType<GameWindowSettings::GraphicAPIHint>();
    TypeRegistry::UnregisterType<GameWindowSettings>();
}

DesktopPlatformModule& DesktopPlatformModule::GetInstance()
{
    static DesktopPlatformModule instance;
    return instance;
}