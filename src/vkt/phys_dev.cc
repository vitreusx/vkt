#include <vkt/phys_dev.h>

PhysicalDevice::PhysicalDevice(Loader const &loader,
                               VkPhysicalDevice physicalDevice) {
  this->physicalDevice = physicalDevice;
  loader.vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  loader.vkGetPhysicalDeviceFeatures(physicalDevice, &features);

  uint32_t queueFamilyCount = 0;
  loader.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                                  &queueFamilyCount, nullptr);

  queueFamilies.resize(queueFamilyCount);
  loader.vkGetPhysicalDeviceQueueFamilyProperties(
      physicalDevice, &queueFamilyCount, queueFamilies.data());

  loader.vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProps);
}

PhysicalDevice::operator VkPhysicalDevice() {
  return physicalDevice;
}