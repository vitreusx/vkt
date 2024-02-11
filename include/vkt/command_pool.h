#pragma once
#include <vkt/device.h>

struct CommandPoolCreateInfo {
  VkCommandPoolCreateFlags flags;
  uint32_t queueFamilyIndex;
};

class CommandPool {
public:
  CommandPool(std::shared_ptr<Device> device,
              CommandPoolCreateInfo const &createInfo);

  CommandPool(CommandPool const &) = delete;
  CommandPool &operator=(CommandPool const &) = delete;

  CommandPool(CommandPool &&other);

  CommandPool &operator=(CommandPool &&other);

  ~CommandPool();

  void destroy();

  operator VkCommandPool();

private:
  std::shared_ptr<Device> device = {};
  VkCommandPool commandPool = VK_NULL_HANDLE;
};