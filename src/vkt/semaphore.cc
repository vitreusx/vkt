#include <vkt/semaphore.h>

Semaphore::Semaphore(std::shared_ptr<Device> device) {
  this->device = device;
  auto vk_createInfo =
      VkSemaphoreCreateInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = {}};

  VkSemaphore semaphore;
  VK_CHECK(
      device->vkCreateSemaphore(*device, &vk_createInfo, nullptr, &semaphore));

  this->semaphore = Handle<VkSemaphore, Device>(
      semaphore,
      [](VkSemaphore semaphore, Device &device) -> void {
        device.vkDestroySemaphore(device, semaphore, nullptr);
      },
      device);
}

Semaphore::operator VkSemaphore() {
  return semaphore;
}