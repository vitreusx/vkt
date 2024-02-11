#pragma once
#include <vkt/device.h>

class Queue {
public:
  Queue() = default;
  Queue(std::shared_ptr<Device> device, uint32_t queueFamilyIndex,
        uint32_t queueIndex);

  Queue(Queue const &) = delete;
  Queue &operator=(Queue const &) = delete;

  Queue(Queue &&other);
  Queue &operator=(Queue &&other);

  operator VkQueue();

  void submit(QueueSubmitInfo const &submitInfo);

  VkResult present(QueuePresentInfo const &presentInfo);

private:
  std::shared_ptr<Device> device = {};
  VkQueue queue = VK_NULL_HANDLE;
};