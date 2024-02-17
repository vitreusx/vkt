#pragma once
#include <vkt/device.h>

struct CommandPoolCreateInfo {
  VkCommandPoolCreateFlags flags;
  uint32_t queueFamilyIndex;
};

class CommandPool {
public:
  CommandPool() = default;
  CommandPool(std::shared_ptr<Device> device,
              CommandPoolCreateInfo const &createInfo);

  operator VkCommandPool();

private:
  std::shared_ptr<Device> device = {};
  Handle<VkCommandPool, Device> commandPool;
};