#include <vkt/semaphore.h>

Semaphore::Semaphore(std::shared_ptr<Device> device) {
  this->device = device;
  auto vk_createInfo =
      VkSemaphoreCreateInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = {}};
  VK_CHECK(
      device->vkCreateSemaphore(*device, &vk_createInfo, nullptr, &semaphore));
}

Semaphore::Semaphore(Semaphore &&other) {
  *this = std::move(other);
}

Semaphore &Semaphore::operator=(Semaphore &&other) {
  destroy();
  device = std::move(other.device);
  semaphore = std::move(other.semaphore);
  other.semaphore = VK_NULL_HANDLE;
  return *this;
}

Semaphore::~Semaphore() {
  destroy();
}

void Semaphore::destroy() {
  if (semaphore != VK_NULL_HANDLE)
    device->vkDestroySemaphore(*device, semaphore, nullptr);
  semaphore = VK_NULL_HANDLE;
}

Semaphore::operator VkSemaphore() {
  return semaphore;
}