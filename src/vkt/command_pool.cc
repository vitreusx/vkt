#include <vkt/command_pool.h>

CommandPool::CommandPool(std::shared_ptr<Device> device,
                         CommandPoolCreateInfo const &createInfo) {
  this->device = device;

  auto vk_createInfo = VkCommandPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = createInfo.flags,
      .queueFamilyIndex = createInfo.queueFamilyIndex};

  VkCommandPool commandPool;
  VK_CHECK(device->vkCreateCommandPool(*device, &vk_createInfo, nullptr,
                                       &commandPool));

  this->commandPool = Handle<VkCommandPool, Device>(
      commandPool,
      [](VkCommandPool commandPool, Device &device) -> void {
        device.vkDestroyCommandPool(device, commandPool, nullptr);
      },
      device);
}

CommandPool::operator VkCommandPool() {
  return commandPool;
}
