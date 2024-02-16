#pragma once
#include <vkt/loader.h>
#include <vector>
#include <optional>
#include <memory>

class PhysicalDevice {
public:
  PhysicalDevice() = default;
  PhysicalDevice(std::shared_ptr<Loader> loader,
                 VkPhysicalDevice physicalDevice);

  operator VkPhysicalDevice();

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  std::vector<VkQueueFamilyProperties> queueFamilies;
  VkPhysicalDeviceMemoryProperties memoryProps;

  std::optional<uint32_t>
  findMemoryTypeIndex(uint32_t memoryTypeBits,
                      VkMemoryPropertyFlags requiredProperties);

  std::optional<VkFormat>
  findSuitableFormat(std::vector<VkFormat> const &candidates,
                     VkFormatFeatureFlags requiredFeatures,
                     bool optimal = true);

private:
  std::shared_ptr<Loader> loader = {};
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
};