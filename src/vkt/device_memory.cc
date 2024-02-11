#include <vkt/device_memory.h>
#include <functional>

DeviceMemory::DeviceMemory(std::shared_ptr<Device> device,
                           MemoryAllocateInfo const &allocInfo) {

  this->device = device;
  this->allocationSize = allocInfo.size;

  auto vk_allocInfo =
      VkMemoryAllocateInfo{.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                           .pNext = nullptr,
                           .allocationSize = allocInfo.size,
                           .memoryTypeIndex = allocInfo.memoryTypeIndex};

  VK_CHECK(
      device->vkAllocateMemory(*device, &vk_allocInfo, nullptr, &deviceMemory));
}

DeviceMemory::DeviceMemory(DeviceMemory &&other) {
  *this = std::move(other);
}

DeviceMemory &DeviceMemory::operator=(DeviceMemory &&other) {
  destroy();
  device = std::move(other.device);
  deviceMemory = other.deviceMemory;
  other.deviceMemory = VK_NULL_HANDLE;
  allocationSize = other.allocationSize;
  return *this;
}

DeviceMemory::~DeviceMemory() {
  destroy();
}

DeviceMemory::operator VkDeviceMemory() {
  return deviceMemory;
}

void DeviceMemory::destroy() {
  if (deviceMemory != VK_NULL_HANDLE) {
    device->vkFreeMemory(*device, deviceMemory, nullptr);
  }
  deviceMemory = VK_NULL_HANDLE;
}

std::shared_ptr<void> DeviceMemory::map(VkDeviceSize offset,
                                        VkDeviceSize size) {
  size = std::min(size, allocationSize);

  void *ptr;
  VK_CHECK(device->vkMapMemory(*device, deviceMemory, offset, size, {}, &ptr));

  auto device = this->device;
  auto deviceMemory = this->deviceMemory;
  return std::shared_ptr<void>(ptr, [this](void *ptr) -> void {
    this->unmapMemory(ptr);
  });
}

void DeviceMemory::unmapMemory(void *ptr) {
  device->vkUnmapMemory(*device, deviceMemory);
}