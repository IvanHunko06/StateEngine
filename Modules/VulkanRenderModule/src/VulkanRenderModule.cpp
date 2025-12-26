#include "VulkanRenderModule/VulkanRenderModule.hpp"
#include "EngineCore/EngineTypeSystem/CompileTimeTypeBuilder.hpp"
#include "EngineCore/EngineTypeSystem/TypeInfo.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistry.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "VulkanRenderModule/Configurations/IVulkanSurfaceProvider.hpp"
#include "VulkanRenderModule/Configurations/OverrideActiveVulkanDevice.hpp"
#include "VulkanRenderModule/Configurations/VulkanCreateInstanceConfiguration.hpp"

using namespace StateEngine::VulkanRenderModule;
using namespace StateEngine::EngineCore::EngineTypeSystem;

void VulkanRenderModule::OnLoad()
{
    if (!vulkanInfo_.initialize()) {
        ZLOG_ERROR("VulkanRenderModule") << "Failed to initialize Vulkan Info!";
        return;
    }
    if (!vulkanRenderDevice_.initialize(vulkanInfo_)) {
        ZLOG_ERROR("VulkanRenderModule") << "Failed to initialize Vulkan Render Device!";
        return;
    }
}
void VulkanRenderModule::OnUnload()
{
    vulkanRenderDevice_.shutdown();
    vulkanInfo_.shutdown();
}

void VulkanRenderModule::RegisterTypes()
{
    using VkResultTypeBuild = CompileTimeTypeBuilder<VkResult>::AutoEnum<-13, 5>::AutoEnum<-1000023005, -1000023000>::
        AutoEnum<1000268000, 1000268003>::EnumValues<
            // --- Validation & Memory ---
            VK_ERROR_VALIDATION_FAILED, VK_ERROR_OUT_OF_POOL_MEMORY, VK_ERROR_FRAGMENTATION, VK_ERROR_NOT_PERMITTED,

            // --- External Memory & Handles ---
            VK_ERROR_INVALID_EXTERNAL_HANDLE, VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS,

            // --- Surface & Swapchain (KHR) ---
            VK_ERROR_SURFACE_LOST_KHR, VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, VK_SUBOPTIMAL_KHR, VK_ERROR_OUT_OF_DATE_KHR,
            VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR,

            // --- Pipelines & Shaders ---
            VK_PIPELINE_COMPILE_REQUIRED, VK_ERROR_INVALID_SHADER_NV, VK_INCOMPATIBLE_SHADER_BINARY_EXT,
            VK_PIPELINE_BINARY_MISSING_KHR, VK_ERROR_NOT_ENOUGH_SPACE_KHR,

            // --- Other Extensions ---
            VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT, VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT,
            VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR, VK_ERROR_COMPRESSION_EXHAUSTED_EXT>;

    TypeRegistry::RegisterType(VkResultTypeBuild::Build(), TypeKind::Enum);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<VkSystemAllocationScope>::AutoEnum<0, 5>::Build(),
                               TypeKind::Enum);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<IVulkanSurfaceProvider>::Build(), TypeKind::Interface);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<VulkanCreateInstanceConfiguration>::Build(), TypeKind::Struct);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<OverrideActiveVulkanDevice>::Build(), TypeKind::Struct);

    using VkPresentModeTypeBuilder = CompileTimeTypeBuilder<VkPresentModeKHR>::AutoEnum<0, 5>::EnumValues<
        VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR, VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR,
        VK_PRESENT_MODE_FIFO_LATEST_READY_KHR>;

    TypeRegistry::RegisterType(VkPresentModeTypeBuilder::Build(), TypeKind::Enum);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<VkFormat>::AutoEnum<0, 124>::Build(), TypeKind::Enum);

    TypeRegistry::RegisterType(CompileTimeTypeBuilder<VkColorSpaceKHR>::AutoEnum<1000104001, 1000104015>::EnumValue<
                                   VK_COLOR_SPACE_SRGB_NONLINEAR_KHR>::Build(),
                               TypeKind::Enum);
    
    TypeRegistry::RegisterType(CompileTimeTypeBuilder<VkSurfaceFormatKHR>::Field<
                                   &VkSurfaceFormatKHR::colorSpace>::Field<&VkSurfaceFormatKHR::format>::Build(),
                               TypeKind::Struct);
}
void VulkanRenderModule::UnregisterTypes()
{
    TypeRegistry::UnregisterType<VkResult>();
    TypeRegistry::UnregisterType<VkSystemAllocationScope>();
    TypeRegistry::UnregisterType<IVulkanSurfaceProvider>();
    TypeRegistry::UnregisterType<VkPresentModeKHR>();
    TypeRegistry::UnregisterType<VkSurfaceFormatKHR>();
    TypeRegistry::UnregisterType<VkFormat>();
    TypeRegistry::UnregisterType<VkColorSpaceKHR>();
    TypeRegistry::UnregisterType<OverrideActiveVulkanDevice>();
    TypeRegistry::UnregisterType<VulkanCreateInstanceConfiguration>();
}