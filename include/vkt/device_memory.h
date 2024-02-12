#pragma once
#include <vkt/device.h>
#include <memory>
#include <limits>

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

  static std::shared_ptr<void>
  map(std::shared_ptr<DeviceMemory> deviceMemory, VkDeviceSize offset = 0,
      VkDeviceSize size = std::numeric_limits<VkDeviceSize>::max());

private:
  std::shared_ptr<Device> device = {};
  VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
  VkDeviceSize allocationSize = 0;

  void destroy();

  std::weak_ptr<void> memoryMap;
  void unmapMemory();
};
