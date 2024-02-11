#include <vkt/fence.h>

Fence::Fence(std::shared_ptr<Device> device, bool signalled) {
  this->device = device;
  auto vk_createInfo = VkFenceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = signalled ? VK_FENCE_CREATE_SIGNALED_BIT : VkFenceCreateFlags{}};
  VK_CHECK(device->vkCreateFence(*device, &vk_createInfo, nullptr, &fence));
}

Fence::Fence(Fence &&other) {
  *this = std::move(other);
}

Fence &Fence::operator=(Fence &&other) {
  destroy();
  device = std::move(other.device);
  fence = std::move(other.fence);
  other.fence = VK_NULL_HANDLE;
  return *this;
}

Fence::~Fence() {
  destroy();
}

void Fence::destroy() {
  if (fence != VK_NULL_HANDLE)
    device->vkDestroyFence(*device, fence, nullptr);
  fence = VK_NULL_HANDLE;
}

Fence::operator VkFence() {
  return fence;
}

void Fence::wait() {
  VK_CHECK(device->vkWaitForFences(*device, 1, &fence, VK_TRUE, UINT64_MAX));
}

void Fence::reset() {
  VK_CHECK(device->vkResetFences(*device, 1, &fence));
}
