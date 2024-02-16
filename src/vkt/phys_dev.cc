#include <vkt/phys_dev.h>

PhysicalDevice::PhysicalDevice(std::shared_ptr<Loader> loader,
                               VkPhysicalDevice physicalDevice) {
  this->physicalDevice = physicalDevice;
  this->loader = loader;

  loader->vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  loader->vkGetPhysicalDeviceFeatures(physicalDevice, &features);

  uint32_t queueFamilyCount = 0;
  loader->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                                   &queueFamilyCount, nullptr);

  queueFamilies.resize(queueFamilyCount);
  loader->vkGetPhysicalDeviceQueueFamilyProperties(
      physicalDevice, &queueFamilyCount, queueFamilies.data());

  loader->vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProps);
}

PhysicalDevice::operator VkPhysicalDevice() {
  return physicalDevice;
}

std::optional<uint32_t>
PhysicalDevice::findMemoryTypeIndex(uint32_t memoryTypeBits,
                                    VkMemoryPropertyFlags requiredProperties) {
  for (uint32_t index = 0; index < memoryProps.memoryTypeCount; ++index) {
    auto const &memoryType = memoryProps.memoryTypes[index];
    if (memoryTypeBits & ((uint32_t)1 << index) == 0)
      continue;
    if ((memoryType.propertyFlags & requiredProperties) != requiredProperties)
      continue;
    return index;
  }
  return std::nullopt;
}

std::optional<VkFormat>
PhysicalDevice::findSuitableFormat(std::vector<VkFormat> const &candidates,
                                   VkFormatFeatureFlags requiredFeatures,
                                   bool optimal) {
  for (auto format : candidates) {
    VkFormatProperties props;
    loader->vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

    if (optimal) {
      if (props.optimalTilingFeatures & requiredFeatures == requiredFeatures)
        return format;
    } else {
      if (props.linearTilingFeatures & requiredFeatures == requiredFeatures)
        return format;
    }
  }

  return std::nullopt;
}