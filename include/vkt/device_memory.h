#pragma once

#include <vkt/device.h>
#include <memory>

struct MemoryAllocateInfo {
  VkDeviceSize size;
  uint32_t memoryTypeIndex;
};

class DeviceMemory {
public:
  DeviceMemory(std::shared_ptr<Device> device,
               MemoryAllocateInfo const &allocInfo);

  DeviceMemory(DeviceMemory const &) = delete;
  DeviceMemory &operator=(DeviceMemory const &) = delete;

  DeviceMemory(DeviceMemory &&other);
  DeviceMemory &operator=(DeviceMemory &&other);

  ~DeviceMemory();

  operator VkDeviceMemory();

  std::shared_ptr<void> map(VkDeviceSize offset = 0,
                            VkDeviceSize size = UINT64_MAX);

private:
  std::shared_ptr<Device> device = {};
  VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
  VkDeviceSize allocationSize = 0;

  void destroy();

  void unmapMemory(void *ptr);
};
