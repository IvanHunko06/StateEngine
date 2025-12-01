#include "ZFrameManager.hpp"
#include "ZVulkanRenderDevice.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistryMacros.hpp"

using namespace FrameManagment;
using namespace StateEngine::EngineCore::EngineTypeSystem;

bool ZFrameManager::initialize(const ZVulkanRenderDevice& renderDevice, uint32_t workerThreadsCount) {
	device_ = renderDevice.logicalDevice_;
	maxFramesInFlight_ = renderDevice.swapchainManager_.getImagesCount();
	workerThreadsCount_ = workerThreadsCount;
	vkResultTypeInfo_ = GET_TYPE_INFO("VulkanRenderModule::VkResult");
	if (!vkResultTypeInfo_) {
		ZLOG_ERROR("VulkanRenderModule") << "Failed to get required type VkResult info";
		return false;
	}
	frames_.resize(maxFramesInFlight_);
	for (auto& frame : frames_) {
		//frame.inFlightFence = renderDevice
		for (auto threadResource : frame.threadResources) {
			//threadResource.computePool.
		}
	}
	return true;
}
void ZFrameManager::shutdown() {

}