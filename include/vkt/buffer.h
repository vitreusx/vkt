#pragma once
#include <vkt/device.h>
#include <set>
#include <vkt/queue.h>
#include <vkt/device_memory.h>

struct BufferCreateInfo {
  VkDeviceSize size;
  VkBufferUsageFlags usage;
  VkSharingMode sharingMode;
  std::set<uint32_t> queueFamilyIndices;
};

class Buffer {
public:
  Buffer() = default;
  Buffer(std::shared_ptr<Device> device, BufferCreateInfo const &createInfo);

  Buffer(Buffer const &) = delete;
  Buffer &operator=(Buffer const &) = delete;

  Buffer(Buffer &&other);
  Buffer &operator=(Buffer &&other);

  void destroy();
  ~Buffer();

  operator VkBuffer();

  DeviceMemory allocMemory(VkMemoryPropertyFlags properties);

  VkMemoryRequirements getMemoryRequirements();
  void stage(void *data, VkDeviceSize size, Queue &transferQueue);

private:
  std::shared_ptr<Device> device = {};
  VkBuffer buffer = VK_NULL_HANDLE;
};