#pragma once
#include <vkt/loader.h>
#include <vector>

class PhysicalDevice {
public:
  PhysicalDevice() = default;
  PhysicalDevice(Loader const &loader, VkPhysicalDevice physicalDevice);

  operator VkPhysicalDevice();

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  std::vector<VkQueueFamilyProperties> queueFamilies;
  VkPhysicalDeviceMemoryProperties memoryProps;

private:
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
};