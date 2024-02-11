#pragma once
#include <vkt/device.h>

struct BufferCreateInfo {
  VkDeviceSize size;
  VkBufferUsageFlags usage;
  VkSharingMode sharingMode;
  std::vector<uint32_t> queueFamilyIndices;
};

class Buffer {
public:
  Buffer(std::shared_ptr<Device> device, BufferCreateInfo const &createInfo);

  Buffer(Buffer const &) = delete;
  Buffer &operator=(Buffer const &) = delete;

  Buffer(Buffer &&other);
  Buffer &operator=(Buffer &&other);

  void destroy();
  ~Buffer();

  operator VkBuffer();

  VkMemoryRequirements getMemoryRequirements();

private:
  std::shared_ptr<Device> device = {};
  VkBuffer buffer = VK_NULL_HANDLE;
};