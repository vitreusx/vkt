#pragma once
#include <vkt/device.h>

class Semaphore {
public:
  Semaphore() = default;
  Semaphore(std::shared_ptr<Device> device);

  Semaphore(Semaphore const &) = delete;
  Semaphore &operator=(Semaphore const &) = delete;

  Semaphore(Semaphore &&other);

  Semaphore &operator=(Semaphore &&other);

  ~Semaphore();

  operator VkSemaphore();

private:
  std::shared_ptr<Device> device = {};
  VkSemaphore semaphore = VK_NULL_HANDLE;

  void destroy();
};