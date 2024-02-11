#pragma once
#include <vkt/device.h>

class Fence {
public:
  Fence() = default;

  Fence(std::shared_ptr<Device> device, bool signalled = false);

  Fence(Fence const &) = delete;
  Fence &operator=(Fence const &) = delete;

  Fence(Fence &&other);
  Fence &operator=(Fence &&other);

  ~Fence();
  void destroy();

  operator VkFence();

  void wait();
  void reset();

private:
  std::shared_ptr<Device> device = {};
  VkFence fence = VK_NULL_HANDLE;
};