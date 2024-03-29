#pragma once
#include <vkt/device.h>
#include <vkt/device_memory.h>
#include <set>
#include <vkt/queue.h>

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

  operator VkImage();

  VkMemoryRequirements getMemoryRequirements();

  DeviceMemory &allocMemory(VkMemoryPropertyFlags properties);
  void bindMemory(std::shared_ptr<DeviceMemory> deviceMemory,
                  VkDeviceSize offset = 0);

  void stage(void *data, VkDeviceSize size, Queue &transferQueue,
             VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask,
             VkImageLayout dstLayout);

  ImageCreateInfo createInfo = {};

private:
  std::shared_ptr<Device> device = {};
  Handle<VkImage, Device> image;
  std::shared_ptr<DeviceMemory> imageMemory;
  VkImageFormatProperties formatProps;
};