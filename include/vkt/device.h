#pragma once
#include <vkt/instance.h>

struct DeviceQueueCreateInfo {
  VkDeviceQueueCreateFlags flags = {};
  uint32_t queueFamilyIndex = {};
  std::vector<float> queuePriorities = {};
};

struct DeviceCreateInfo {
  VkDeviceCreateFlags flags = {};
  std::vector<DeviceQueueCreateInfo> queueCreateInfos = {};
  std::vector<std::string> enabledLayers = {};
  std::vector<std::string> enabledExtensions = {};
  VkPhysicalDeviceFeatures enabledFeatures = {};
};

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

class Device {
public:
  Device(VkDevice &&device);
  Device(std::shared_ptr<Loader> loader, VkPhysicalDevice physicalDevice,
         DeviceCreateInfo const &deviceCreateInfo);

  Device(Device const &) = delete;
  Device &operator=(Device const &) = delete;

  Device(Device &&) = delete;
  Device &operator=(Device &&) = delete;

  ~Device();

  operator VkDevice();

public:
#define MEMBER(name) PFN_##name name
  MEMBER(vkGetDeviceQueue);
  MEMBER(vkCreateSwapchainKHR);
  MEMBER(vkDestroySwapchainKHR);
  MEMBER(vkGetSwapchainImagesKHR);
  MEMBER(vkCreateImageView);
  MEMBER(vkDestroyImageView);
  MEMBER(vkCreateShaderModule);
  MEMBER(vkDestroyShaderModule);
  MEMBER(vkCreatePipelineLayout);
  MEMBER(vkDestroyPipelineLayout);
  MEMBER(vkCreateGraphicsPipelines);
  MEMBER(vkDestroyPipeline);
  MEMBER(vkCreateRenderPass);
  MEMBER(vkDestroyRenderPass);
  MEMBER(vkCreateFramebuffer);
  MEMBER(vkDestroyFramebuffer);
  MEMBER(vkCreateCommandPool);
  MEMBER(vkDestroyCommandPool);
  MEMBER(vkAllocateCommandBuffers);
  MEMBER(vkFreeCommandBuffers);
  MEMBER(vkCreateSemaphore);
  MEMBER(vkDestroySemaphore);
  MEMBER(vkCreateFence);
  MEMBER(vkDestroyFence);
  MEMBER(vkWaitForFences);
  MEMBER(vkResetFences);
  MEMBER(vkAcquireNextImageKHR);
  MEMBER(vkQueueSubmit);
  MEMBER(vkQueuePresentKHR);
  MEMBER(vkCreateBuffer);
  MEMBER(vkDestroyBuffer);
  MEMBER(vkDeviceWaitIdle);
  MEMBER(vkAllocateMemory);
  MEMBER(vkFreeMemory);
  MEMBER(vkBindBufferMemory);
  MEMBER(vkMapMemory);
  MEMBER(vkUnmapMemory);
#undef MEMBER

  void loadFunctions();

private:
  std::shared_ptr<Loader> loader = {};
  VkDevice device = VK_NULL_HANDLE;
};