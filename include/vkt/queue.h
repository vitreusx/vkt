#pragma once
#include <vkt/device.h>

struct QueueSubmitInfo {
  std::vector<std::pair<VkSemaphore, VkPipelineStageFlags>>
      waitSemaphoresAndStages;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> signalSemaphores;
  VkFence fence;
};

struct QueuePresentInfo {
  std::vector<VkSemaphore> waitSemaphores;
  std::vector<std::pair<VkSwapchainKHR, uint32_t>> swapchainsAndImageIndices;
};

class Queue {
public:
  Queue() = default;
  Queue(std::shared_ptr<Device> device, uint32_t queueFamilyIndex,
        uint32_t queueIndex);

  Queue(Queue const &) = default;
  Queue &operator=(Queue const &) = default;

  Queue(Queue &&other);
  Queue &operator=(Queue &&other);

  operator VkQueue();

  void submit(QueueSubmitInfo const &submitInfo);

  VkResult present(QueuePresentInfo const &presentInfo);

  void wait();

  uint32_t getQueueFamilyIndex() const;
  uint32_t getQueueIndex() const;

private:
  std::shared_ptr<Device> device = {};
  VkQueue queue = VK_NULL_HANDLE;
  uint32_t queueFamilyIndex = 0, queueIndex = 0;
};