#pragma once
#include <vkt/device.h>
#include <memory>
#include <limits>

struct MemoryAllocateInfo {
  VkDeviceSize size;
  uint32_t memoryTypeIndex;
};

class DeviceMemoryMap;

class DeviceMemory {
public:
  DeviceMemory() = default;
  DeviceMemory(std::shared_ptr<Device> device,
               MemoryAllocateInfo const &allocInfo);

  operator VkDeviceMemory();

  std::shared_ptr<void>
  map(VkDeviceSize offset = 0,
      VkDeviceSize size = std::numeric_limits<VkDeviceSize>::max());

private:
  friend class DeviceMemoryMap;

  std::shared_ptr<Device> device = {};
  Handle<VkDeviceMemory, Device> deviceMemory = {};
  VkDeviceSize allocationSize = 0;

  std::weak_ptr<void> memoryMap;
};
