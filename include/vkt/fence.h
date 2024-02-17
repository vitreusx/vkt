#pragma once
#include <vkt/device.h>

class Fence {
public:
  Fence() = default;
  Fence(std::shared_ptr<Device> device, bool signalled = false);

  operator VkFence();

  void wait();
  void reset();

private:
  std::shared_ptr<Device> device = {};
  Handle<VkFence, Device> fence;
};