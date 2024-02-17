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

private:
  friend class DeviceMemoryMap;

  std::shared_ptr<Device> device = {};
  Handle<VkDeviceMemory, Device> deviceMemory = {};
  VkDeviceSize allocationSize = 0;

  std::weak_ptr<void> memoryMap;
  void unmapMemory();
};

class DeviceMemoryMap {
public:
  DeviceMemoryMap() = default;
  DeviceMemoryMap(std::shared_ptr<DeviceMemory> deviceMemory,
                  VkDeviceSize offset = 0,
                  VkDeviceSize size = std::numeric_limits<VkDeviceSize>::max());

  DeviceMemoryMap(DeviceMemoryMap &&other);
  DeviceMemoryMap &operator=(DeviceMemoryMap &&other);

  ~DeviceMemoryMap();

  void *get() const;

private:
  std::shared_ptr<DeviceMemory> deviceMemory = {};
  std::shared_ptr<void> ptr = nullptr;
};