#include <vkt/queue.h>

Queue::Queue(std::shared_ptr<Device> device, uint32_t queueFamilyIndex,
             uint32_t queueIndex) {
  this->device = device;
  this->queueFamilyIndex = queueFamilyIndex;
  this->queueIndex = queueIndex;
  device->vkGetDeviceQueue(*device, queueFamilyIndex, queueIndex, &queue);
}

Queue::Queue(Queue &&other) {
  *this = std::move(other);
}

Queue &Queue::operator=(Queue &&other) {
  device = std::move(other.device);
  queue = other.queue;
  other.queue = VK_NULL_HANDLE;
  return *this;
}

Queue::operator VkQueue() {
  return queue;
}

void Queue::submit(QueueSubmitInfo const &submitInfo) {
  std::vector<VkSemaphore> waitSemaphores;
  std::vector<VkPipelineStageFlags> waitDstStageMasks;
  for (auto const &[semaphore, stage] : submitInfo.waitSemaphoresAndStages) {
    waitSemaphores.push_back(semaphore);
    waitDstStageMasks.push_back(stage);
  }

  auto vk_submitInfo = VkSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = (uint32_t)waitSemaphores.size(),
      .pWaitSemaphores = waitSemaphores.data(),
      .pWaitDstStageMask = waitDstStageMasks.data(),
      .commandBufferCount = (uint32_t)submitInfo.commandBuffers.size(),
      .pCommandBuffers = submitInfo.commandBuffers.data(),
      .signalSemaphoreCount = (uint32_t)submitInfo.signalSemaphores.size(),
      .pSignalSemaphores = submitInfo.signalSemaphores.data()};

  VK_CHECK(device->vkQueueSubmit(queue, 1, &vk_submitInfo, submitInfo.fence));
}

VkResult Queue::present(QueuePresentInfo const &presentInfo) {
  std::vector<VkSwapchainKHR> swapchains;
  std::vector<uint32_t> imageIndices;
  for (auto const &[swapchain, imageIndex] :
       presentInfo.swapchainsAndImageIndices) {
    swapchains.push_back(swapchain);
    imageIndices.push_back(imageIndex);
  }

  auto vk_presentInfo = VkPresentInfoKHR{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = (uint32_t)presentInfo.waitSemaphores.size(),
      .pWaitSemaphores = presentInfo.waitSemaphores.data(),
      .swapchainCount = (uint32_t)swapchains.size(),
      .pSwapchains = swapchains.data(),
      .pImageIndices = imageIndices.data(),
      .pResults = nullptr};

  return device->vkQueuePresentKHR(queue, &vk_presentInfo);
}

void Queue::wait() {
  VK_CHECK(device->vkQueueWaitIdle(queue));
}

uint32_t Queue::getQueueFamilyIndex() const {
  return queueFamilyIndex;
}

uint32_t Queue::getQueueIndex() const {
  return queueIndex;
}