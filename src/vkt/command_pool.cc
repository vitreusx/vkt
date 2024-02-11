#include <vkt/command_pool.h>

CommandPool::CommandPool(std::shared_ptr<Device> device,
                         CommandPoolCreateInfo const &createInfo) {
  this->device = device;

  auto vk_createInfo = VkCommandPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = createInfo.flags,
      .queueFamilyIndex = createInfo.queueFamilyIndex};

  VK_CHECK(device->vkCreateCommandPool(*device, &vk_createInfo, nullptr,
                                       &commandPool));
}

CommandPool::CommandPool(CommandPool &&other) {
  *this = std::move(other);
}

CommandPool &CommandPool::operator=(CommandPool &&other) {
  destroy();
  device = std::move(other.device);
  commandPool = other.commandPool;
  other.commandPool = VK_NULL_HANDLE;
  return *this;
}

CommandPool::~CommandPool() {
  destroy();
}

void CommandPool::destroy() {
  if (commandPool != VK_NULL_HANDLE)
    device->vkDestroyCommandPool(*device, commandPool, nullptr);
  commandPool = VK_NULL_HANDLE;
}

CommandPool::operator VkCommandPool() {
  return commandPool;
}
