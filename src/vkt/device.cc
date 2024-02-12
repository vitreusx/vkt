#include <vkt/device.h>

Device::Device(VkDevice &&device) : device{std::move(device)} {}

Device::Device(std::shared_ptr<Loader> loader, VkPhysicalDevice physicalDevice,
               DeviceCreateInfo const &deviceCreateInfo) {
  this->loader = loader;

  auto vk_queueCreateInfos =
      mapV(deviceCreateInfo.queueCreateInfos,
           [](auto const &queueCreateInfo) -> auto {
             return VkDeviceQueueCreateInfo{
                 .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                 .pNext = VK_NULL_HANDLE,
                 .flags = queueCreateInfo.flags,
                 .queueFamilyIndex = queueCreateInfo.queueFamilyIndex,
                 .queueCount = (uint32_t)queueCreateInfo.queuePriorities.size(),
                 .pQueuePriorities = queueCreateInfo.queuePriorities.data()};
           });

  auto extNames = vkMapNames(deviceCreateInfo.enabledExtensions);
  auto layerNames = vkMapNames(deviceCreateInfo.enabledLayers);

  VkDeviceCreateInfo vk_deviceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = VK_NULL_HANDLE,
      .flags = deviceCreateInfo.flags,
      .queueCreateInfoCount = (uint32_t)vk_queueCreateInfos.size(),
      .pQueueCreateInfos = vk_queueCreateInfos.data(),
      .enabledLayerCount = (uint32_t)layerNames.size(),
      .ppEnabledLayerNames = layerNames.data(),
      .enabledExtensionCount = (uint32_t)extNames.size(),
      .ppEnabledExtensionNames = extNames.data(),
      .pEnabledFeatures = &deviceCreateInfo.enabledFeatures};

  VK_CHECK(loader->vkCreateDevice(physicalDevice, &vk_deviceCreateInfo,
                                  VK_NULL_HANDLE, &device));

  loadFunctions();
}

Device::~Device() {
  if (device != VK_NULL_HANDLE)
    loader->vkDestroyDevice(device, VK_NULL_HANDLE);
  device = VK_NULL_HANDLE;
}

Device::operator VkDevice() {
  return device;
}

void Device::loadFunctions() {
#define LOAD(name) this->name = (PFN_##name)vkGetDeviceProcAddr(device, #name)
  DEVICE_DEFS(LOAD);
#undef LOAD
}