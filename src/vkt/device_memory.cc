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

  VkDeviceMemory deviceMemory;
  VK_CHECK(
      device->vkAllocateMemory(*device, &vk_allocInfo, nullptr, &deviceMemory));

  this->deviceMemory = Handle<VkDeviceMemory, Device>(
      deviceMemory,
      [](VkDeviceMemory deviceMemory, Device &device) -> void {
        device.vkFreeMemory(device, deviceMemory, nullptr);
      },
      device);
}

DeviceMemory::operator VkDeviceMemory() {
  return deviceMemory;
}

std::shared_ptr<void> DeviceMemory::map(VkDeviceSize offset,
                                        VkDeviceSize size) {
  if (!memoryMap.expired())
    return memoryMap.lock();

  size = std::min(size, allocationSize);
  void *ptr;
  VK_CHECK(device->vkMapMemory(*device, deviceMemory, offset, size, {}, &ptr));

  VkDeviceMemory deviceMemory = this->deviceMemory;
  std::shared_ptr<Device> device = this->device;
  auto unmapMemory = [deviceMemory, device](void *ptr) -> void {
    device->vkUnmapMemory(*device, deviceMemory);
  };

  auto memoryMap = std::shared_ptr<void>(ptr, unmapMemory);
  this->memoryMap = memoryMap;
  return memoryMap;
}
