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

void DeviceMemory::unmapMemory() {
  device->vkUnmapMemory(*device, deviceMemory);
}

DeviceMemoryMap::DeviceMemoryMap(std::shared_ptr<DeviceMemory> deviceMemory,
                                 VkDeviceSize offset, VkDeviceSize size) {
  this->deviceMemory = deviceMemory;

  if (!deviceMemory->memoryMap.expired())
    throw std::runtime_error("Device memory already mapped.");

  size = std::min(size, deviceMemory->allocationSize);

  void *ptr;
  VK_CHECK(deviceMemory->device->vkMapMemory(*deviceMemory->device,
                                             deviceMemory->deviceMemory, offset,
                                             size, {}, &ptr));

  this->ptr = std::shared_ptr<void>(ptr, [](void *ptr) -> void {});
}

DeviceMemoryMap::~DeviceMemoryMap() {
  if (ptr.use_count() == 1)
    deviceMemory->unmapMemory();
}

void *DeviceMemoryMap::get() const {
  return ptr.get();
}