#pragma once
#include <vkt/device.h>
#include <vkt/device_memory.h>
#include <set>

struct ImageCreateInfo {
  VkImageCreateFlags flags;
  VkImageType imageType;
  VkFormat format;
  VkExtent3D extent;
  uint32_t mipLevels;
  uint32_t arrayLayers;
  VkSampleCountFlagBits samples;
  VkImageTiling tiling;
  VkImageUsageFlags usage;
  VkSharingMode sharingMode;
  std::set<uint32_t> queueFamilyIndices;
  VkImageLayout initialLayout;
};

class Image {
public:
  Image() = default;

  Image(std::shared_ptr<Device> device, ImageCreateInfo const &createInfo);

  Image(Image const &) = delete;
  Image &operator=(Image const &) = delete;

  Image(Image &&other);
  Image &operator=(Image &&other);

  ~Image();

  operator VkImage();

private:
  std::shared_ptr<Device> device = {};
  VkImage image = VK_NULL_HANDLE;

  void destroy();
};