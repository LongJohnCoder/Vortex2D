//
//  Instance.cpp
//  Vortex2D
//

#include "Instance.h"

#include <vk_loader/vk_loader.h>
#include <iostream>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
                                             VkDebugReportObjectTypeEXT objType,
                                             uint64_t obj,
                                             size_t location,
                                             int32_t code,
                                             const char* layerPrefix,
                                             const char* msg,
                                             void* userData)
{
    std::cout << "validation layer: " << msg << std::endl;
    return VK_FALSE;
}

namespace Vortex2D { namespace Renderer {

void Instance::Create(const std::string& name, std::vector<const char*> extensions, bool validation)
{
    // load symbols
    if (!vkLoaderInit()) throw std::runtime_error("cannot load vulkan library!");

    auto availableLayers = vk::enumerateInstanceLayerProperties();
    auto availableExtensions = vk::enumerateInstanceExtensionProperties();

    // add the validation extension if necessary
    if (validation && HasExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, availableExtensions))
    {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    std::vector<const char*> validationLayers;
    if (validation && HasLayer(VK_LAYER_LUNARG_STANDARD_VALIDATION_NAME, availableLayers))
    {
        validationLayers.push_back(VK_LAYER_LUNARG_STANDARD_VALIDATION_NAME);
    }

    // configure instance
    auto appInfo = vk::ApplicationInfo()
            .setPApplicationName(name.c_str())
            .setApiVersion(VK_MAKE_VERSION(1, 0, 65));

    vk::InstanceCreateInfo instanceInfo;
    instanceInfo
            .setPApplicationInfo(&appInfo)
            .setEnabledExtensionCount((uint32_t)extensions.size())
            .setPpEnabledExtensionNames(extensions.data())
            .setEnabledLayerCount((uint32_t)validationLayers.size())
            .setPpEnabledLayerNames(validationLayers.data());

    mInstance = vk::createInstanceUnique(instanceInfo);

    // load symbols
    if (!vkLoaderInstanceInit(static_cast<VkInstance>(*mInstance))) throw std::runtime_error("cannot load instance procs");

    // add the validation calback if necessary
    if (validation && HasExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, availableExtensions))
    {
        vk::DebugReportCallbackCreateInfoEXT debugCallbackInfo;
        debugCallbackInfo
                .setPfnCallback(debugCallback)
                .setFlags(vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eError);

        mDebugCallback = mInstance->createDebugReportCallbackEXTUnique(debugCallbackInfo);
    }

    // get physical device
    // TODO better search than first available device
    // - using swap chain info
    // - using queue info
    // - discrete GPU
    mPhysicalDevice = mInstance->enumeratePhysicalDevices().at(0);
    auto properties = mPhysicalDevice.getProperties();
    std::cout << "Device name: " << properties.deviceName << std::endl;

}

vk::PhysicalDevice Instance::GetPhysicalDevice() const
{
    return mPhysicalDevice;
}

vk::Instance Instance::GetInstance() const
{
    return *mInstance;
}

bool HasLayer(const char* extension, const std::vector<vk::LayerProperties>& availableExtensions)
{
  auto find_it = std::find_if(availableExtensions.begin(), availableExtensions.end(),
      [&](const vk::LayerProperties& layer)
      {
          return std::strcmp(extension, layer.layerName) == 0;
      });

  return find_it != availableExtensions.end();
}

bool HasExtension(const char* extension, const std::vector<vk::ExtensionProperties>& availableExtensions)
{
  auto find_it = std::find_if(availableExtensions.begin(), availableExtensions.end(),
      [&](const vk::ExtensionProperties& layer)
      {
          return std::strcmp(extension, layer.extensionName) == 0;
      });

  return find_it != availableExtensions.end();
}

}}
