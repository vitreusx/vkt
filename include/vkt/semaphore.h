#pragma once
#include <vkt/device.h>

class Semaphore {
public:
  Semaphore() = default;
  Semaphore(std::shared_ptr<Device> device);

  operator VkSemaphore();

private:
  std::shared_ptr<Device> device = {};
  Handle<VkSemaphore, Device> semaphore;
};