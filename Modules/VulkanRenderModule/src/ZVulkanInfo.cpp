#include "ZVulkanInfo.hpp"
#include "EngineCore/Logging/LoggingMacros.hpp"
#include "EngineCore/EngineTypeSystem/TypeRegistryMacros.hpp"
#include "EngineCore/ServiceLocator/ServiceLocatorExports.hpp"


#ifdef _WIN32
#include <dxgi.h>
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#ifdef ERROR
#undef ERROR
#endif
#endif

#define VOLK_IMPLEMENTATION
#include "volk.h"

using namespace StateEngine::EngineCore::EngineTypeSystem;


bool ZVulkanInfo::initialize() {
	if (!initializeServices()) return false;
	if (!initializeVolk()) return false;

	setupAllocationCallbacks();

#ifdef _WIN32
	warmUpAllDevices();
#endif

	if(!createVulkanInstance())
		return false;

	vulkanSurfaceProvider_->createSurface(instance_, &allocationCallbacks_, reinterpret_cast<uint64_t*>(&surface_));
	if (surface_ == VK_NULL_HANDLE) {
		ZLOG_ERROR("VulkanRenderModule") << "failed to create surface via vulkanSurfaceProvider";
		return false;
	}

	if (!enumeratePhysicalDevices())
		return false;
	
	return true;
}

void ZVulkanInfo::shutdown() {
	if (surface_)
		vkDestroySurfaceKHR(instance_, surface_, &allocationCallbacks_);

	if(instance_)
		vkDestroyInstance(instance_, &allocationCallbacks_);

	volkFinalize();
}
bool ZVulkanInfo::initializeServices() {
	vkResultTypeInfo_ = GET_TYPE_INFO("VulkanRenderModule::VkResult");
	vulkanSurfaceProvider_ = reinterpret_cast<IVulkanSurfaceProvider*>(
		ServiceLocator_GetService(GET_TYPE_INFO("VulkanRenderModule::IVulkanSurfaceProvider"))
		);
	vulkanCreateInstanceConfiguration_ = reinterpret_cast<VulkanCreateInstanceConfiguration*>(
		ServiceLocator_GetService(GET_TYPE_INFO("VulkanRenderModule::VulkanCreateInstanceConfiguration"))
		);
	overrideActiveVulkanDevice = reinterpret_cast<OverrideActiveVulkanDevice*>(
		ServiceLocator_GetService(GET_TYPE_INFO("VulkanRenderModule::OverrideActiveVulkanDevice"))
		);
	if (!vkResultTypeInfo_) {
		ZLOG_ERROR("VulkanRenderModule") << "Failed to get required type VkResult info";
		return false;
	}

	if (!vulkanSurfaceProvider_) {
		ZLOG_ERROR("VulkanRenderModule") << "Failed to get required service IVulkanSurfaceProvider";
		return false;
	}
	if (!vulkanCreateInstanceConfiguration_) {
		ZLOG_ERROR("VulkanRenderModule") << "Failed to get required service VulkanCreateInstanceConfiguration";
		return false;
	}
}

bool ZVulkanInfo::initializeVolk() {
	VkResult res = volkInitialize();
	if (res != VK_SUCCESS) {
		ZLOG_ERROR("VulkanRenderModule") << "Failed to initialize Volk: " << vkResultTypeInfo_->toString(TypeInstance(&res, vkResultTypeInfo_), nullptr).c_str();
		return false;
	}
	return true;
}
void ZVulkanInfo::setupAllocationCallbacks() {
	allocationCallbacks_ = VkAllocationCallbacks{
	.pUserData = nullptr,
	.pfnAllocation = [](void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) -> void* {
		return MemoryAllocator_AlignedAllocate(size, alignment);
	},
	.pfnReallocation = [](void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) -> void* {
		return MemoryAllocator_Realloc(pOriginal, size, alignment);
	},
	.pfnFree = [](void* pUserData, void* pMemory) {
		MemoryAllocator_Deallocate(pMemory);
	},
#ifndef _DEBUG
		.pfnInternalAllocation = nullptr,
		.pfnInternalFree = nullptr
#else
		.pfnInternalAllocation = [](void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {
			const TypeInfo* vkSystemAllocationScopeType = GET_TYPE_INFO("VulkanRenderModule::VkSystemAllocationScope");
			if (vkSystemAllocationScopeType) {
				ZLOG_DEBUG("VulkanAllocationCallbacks") << "Vulkan internal allocation of size " << size << " bytes for allocation scope " << vkSystemAllocationScopeType->toString(TypeInstance(&allocationScope, vkSystemAllocationScopeType), nullptr).c_str() << ".";
			}
		},
		.pfnInternalFree = [](void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {
			const TypeInfo* vkSystemAllocationScopeType = GET_TYPE_INFO("VulkanRenderModule::VkSystemAllocationScope");
			if (vkSystemAllocationScopeType) {
				ZLOG_DEBUG("VulkanAllocationCallbacks") << "Vulkan internal free of size " << size << " bytes for allocation scope " << vkSystemAllocationScopeType->toString(TypeInstance(&allocationScope, vkSystemAllocationScopeType), nullptr).c_str() << ".";
			}
		}
#endif
	};
}
bool ZVulkanInfo::createVulkanInstance() {
	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = vulkanCreateInstanceConfiguration_->applicationName,
		.applicationVersion = VK_MAKE_VERSION(
			vulkanCreateInstanceConfiguration_->applicationVersion.major, 
			vulkanCreateInstanceConfiguration_->applicationVersion.minor, 
			vulkanCreateInstanceConfiguration_->applicationVersion.patch),
		.pEngineName = vulkanCreateInstanceConfiguration_->engineName,
		.engineVersion = VK_MAKE_VERSION(
			vulkanCreateInstanceConfiguration_->engineVersion.major, 
			vulkanCreateInstanceConfiguration_->engineVersion.minor, 
			vulkanCreateInstanceConfiguration_->engineVersion.patch),
		.apiVersion = VK_API_VERSION_1_1
	};

	uint32_t surfaceExtensionsCount = 0;
	vulkanSurfaceProvider_->getSurfaceExtensions(&surfaceExtensionsCount, nullptr);
	ZBuffer<ZString> surfaceExtensions(surfaceExtensionsCount);
	if (surfaceExtensionsCount) {
		vulkanSurfaceProvider_->getSurfaceExtensions(&surfaceExtensionsCount, surfaceExtensions.data());
	}

	ZBuffer<const char*> enabledExtensions;
	for (auto& surfaceExtension : surfaceExtensions) {
		enabledExtensions.push_back(surfaceExtension.c_str());
	}
#ifdef _DEBUG
	enabledExtensions.push_back("VK_EXT_debug_utils");
#endif

	VkInstanceCreateInfo instanceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
		.ppEnabledExtensionNames = enabledExtensions.data()
	};

	ZLOG_DEBUG("VulkanRenderModule") << "Enabling " << enabledExtensions.size()
		<< " instance extensions...";
	for (auto& ext : enabledExtensions) {
		ZLOG_DEBUG("VulkanRenderModule") << "  > " << ext;
	}

	VkResult res = vkCreateInstance(&instanceCreateInfo, &allocationCallbacks_, &instance_);
	if (res != VK_SUCCESS) {
		ZLOG_ERROR("VulkanRenderModule") << "Failed to create Vulkan instance: " << vkResultTypeInfo_->toString(TypeInstance(&res, vkResultTypeInfo_), nullptr).c_str();
		return false;
	}
	volkLoadInstance(instance_);
	return true;
}


bool ZVulkanInfo::enumeratePhysicalDevices() {
	uint32_t deviceCount = 0;
	VkResult res = vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
	if(res != VK_SUCCESS) {
		if (vkResultTypeInfo_)
			ZLOG_ERROR("VulkanRenderModule") << "Failed to enumerate Vulkan physical devices in first call: " << vkResultTypeInfo_->toString(TypeInstance(&res, vkResultTypeInfo_), nullptr).c_str();
		return false;
	}
	if(deviceCount == 0) {
		ZLOG_ERROR("VulkanRenderModule") << "No Vulkan physical devices found.";
		return false;
	}
	physicalDevices_.resize(deviceCount);
	ZBuffer<VkPhysicalDevice> rawHandles(deviceCount);
	res = vkEnumeratePhysicalDevices(instance_, &deviceCount, rawHandles.data());
	if (res != VK_SUCCESS) {
		ZLOG_ERROR("VulkanRenderModule") << "Failed to enumerate Vulkan physical devices in second call: " << vkResultTypeInfo_->toString(TypeInstance(&res, vkResultTypeInfo_), nullptr).c_str();
		return false;
	}
	ZLOG_INFO("VulkanRenderModule") << "Found " << deviceCount << " Vulkan physical devices.";
	size_t index = 0;
	for (auto& rawHandle : rawHandles) {
		auto& curPhysicalDevice = physicalDevices_[index++];
		curPhysicalDevice.physicalDevice = rawHandle;
		curPhysicalDevice.deviceIndex = index - 1;

		vkGetPhysicalDeviceProperties(rawHandle, &curPhysicalDevice.properties);
		ZLOG_INFO("VulkanRenderModule") << "Vulkan Physical Device " << index - 1 << ": " << curPhysicalDevice.properties.deviceName 
			<< " Driver Version: " << curPhysicalDevice.properties.driverVersion 
			<< " API Version: " 
			<< VK_VERSION_MAJOR(curPhysicalDevice.properties.apiVersion) << "." 
			<< VK_VERSION_MINOR(curPhysicalDevice.properties.apiVersion) << "." 
			<< VK_VERSION_PATCH(curPhysicalDevice.properties.apiVersion);

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(rawHandle, &queueFamilyCount, nullptr);
		curPhysicalDevice.queueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(rawHandle, &queueFamilyCount, curPhysicalDevice.queueFamilyProperties.data());

		vkGetPhysicalDeviceMemoryProperties(rawHandle, &curPhysicalDevice.memoryProperties);
		vkGetPhysicalDeviceFeatures(rawHandle, &curPhysicalDevice.supportedFeatures);

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rawHandle, surface_, &curPhysicalDevice.surfaceCapabilities);
		
		uint32_t surfaceFormatsCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(rawHandle, surface_, &surfaceFormatsCount, nullptr);
		if (surfaceFormatsCount) {
			curPhysicalDevice.surfaceFormats.resize(surfaceFormatsCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(rawHandle, surface_, &surfaceFormatsCount, curPhysicalDevice.surfaceFormats.data());
		}
		
		UINT32 presentModesCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(rawHandle, surface_, &presentModesCount, nullptr);
		if (presentModesCount) {
			curPhysicalDevice.presentModes.resize(presentModesCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(rawHandle, surface_, &presentModesCount, curPhysicalDevice.presentModes.data());
		}

		uint32_t extensionsCount;
		res = vkEnumerateDeviceExtensionProperties(rawHandle, nullptr, &extensionsCount, nullptr);
		if (res == VK_SUCCESS && extensionsCount) {
			ZBuffer<VkExtensionProperties> extensionProperties(extensionsCount);
			res = vkEnumerateDeviceExtensionProperties(rawHandle, nullptr, &extensionsCount, extensionProperties.data());
			for (auto& extProp : extensionProperties) {
				curPhysicalDevice.supportedExtensions.insert(ZString(extProp.extensionName));
			}
		}
		else {
			ZLOG_ERROR("VulkanRenderModule") << "Failed to enumerate Vulkan device  " << index - 1 << " extension properties: " << vkResultTypeInfo_->toString(TypeInstance(&res, vkResultTypeInfo_), nullptr).c_str();
		}

	}
	return true;

}

#ifdef _WIN32
void ZVulkanInfo::warmUpAllDevices() {
	IDXGIFactory* factory = nullptr;
	if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory))) {
		IDXGIAdapter* adapter = nullptr;

		for (UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);
			if (desc.VendorId == 5140) continue;

			ZLOG_DEBUG("VulkanRenderModule") << "Creating test D3D11 device to warm up adapter: " << wideToUTF8(desc.Description).c_str();
			ID3D11Device* dev = nullptr;
			ID3D11DeviceContext* ctx = nullptr;
			D3D_FEATURE_LEVEL fl;
			HRESULT hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
				0, nullptr, 0, D3D11_SDK_VERSION, &dev, &fl, &ctx);

			if (SUCCEEDED(hr)) {
				if (ctx) ctx->Release();
				if (dev) dev->Release();
			}

			adapter->Release();
		}
		factory->Release();
	}

}
ZString ZVulkanInfo::wideToUTF8(const wchar_t* wideString) {
	if (wideString == nullptr) {
		return "";
	}
	int requiredSize = WideCharToMultiByte(
		CP_UTF8,
		0,
		wideString,
		-1,
		nullptr,
		0,
		nullptr,
		nullptr
	);

	if (requiredSize == 0) {
		return ""; // Œ¯Ë·Í‡
	}

	ZBuffer<char> utf8Buffer(requiredSize);
	WideCharToMultiByte(
		CP_UTF8,
		0,
		wideString,
		-1,
		utf8Buffer.data(),
		requiredSize,
		nullptr,
		nullptr
	);
	return utf8Buffer.data();
}
#endif

const VulkanPhysicalDevice& ZVulkanInfo::selectActivePhysicalDevice() const noexcept{
	if (overrideActiveVulkanDevice) {
		ZLOG_DEBUG("VulkanRenderModule") << "Found OverrideActiveVulkanDevice configuration. Index of the requested device: " << overrideActiveVulkanDevice->newActiveDevice;
		if (overrideActiveVulkanDevice->newActiveDevice < physicalDevices_.size()) {
			return physicalDevices_[overrideActiveVulkanDevice->newActiveDevice];
		}
		ZLOG_WARN("VulkanRenderModule") << "Invalid override index. The standard mechanism will be used.";
	}
	for (auto& device : physicalDevices_) {
		if(device.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			return device;
		}
	}
	return physicalDevices_[0];
}