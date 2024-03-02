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

  DeviceMemory &allocMemory(VkMemoryPropertyFlags properties);
  void bindMemory(std::shared_ptr<DeviceMemory> deviceMemory,
                  VkDeviceSize offset = 0);

  VkMemoryRequirements getMemoryRequirements();
  void stage(const void *data, VkDeviceSize size, Queue &transferQueue);

  operator VkBuffer();

private:
  std::shared_ptr<Device> device = {};
  Handle<VkBuffer, Device> buffer = {};
  std::shared_ptr<DeviceMemory> memory;
};