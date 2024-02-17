#include <vkt/fence.h>

Fence::Fence(std::shared_ptr<Device> device, bool signalled) {
  this->device = device;
  auto vk_createInfo = VkFenceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = signalled ? VK_FENCE_CREATE_SIGNALED_BIT : VkFenceCreateFlags{}};

  VkFence fence;
  VK_CHECK(device->vkCreateFence(*device, &vk_createInfo, nullptr, &fence));

  this->fence = Handle<VkFence, Device>(
      fence,
      [](VkFence fence, Device &device) -> void {
        device.vkDestroyFence(device, fence, nullptr);
      },
      device);
}

Fence::operator VkFence() {
  return fence;
}

void Fence::wait() {
  VK_CHECK(device->vkWaitForFences(*device, 1, &(VkFence &)fence, VK_TRUE,
                                   UINT64_MAX));
}

void Fence::reset() {
  VK_CHECK(device->vkResetFences(*device, 1, &(VkFence &)fence));
}
