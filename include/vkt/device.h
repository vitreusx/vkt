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
#define DEVICE_DEFS(MACRO)                                                     \
  MACRO(vkGetDeviceQueue);                                                     \
  MACRO(vkCreateSwapchainKHR);                                                 \
  MACRO(vkDestroySwapchainKHR);                                                \
  MACRO(vkGetSwapchainImagesKHR);                                              \
  MACRO(vkCreateImageView);                                                    \
  MACRO(vkDestroyImageView);                                                   \
  MACRO(vkCreateShaderModule);                                                 \
  MACRO(vkDestroyShaderModule);                                                \
  MACRO(vkCreatePipelineLayout);                                               \
  MACRO(vkDestroyPipelineLayout);                                              \
  MACRO(vkCreateGraphicsPipelines);                                            \
  MACRO(vkDestroyPipeline);                                                    \
  MACRO(vkCreateRenderPass);                                                   \
  MACRO(vkDestroyRenderPass);                                                  \
  MACRO(vkCreateFramebuffer);                                                  \
  MACRO(vkDestroyFramebuffer);                                                 \
  MACRO(vkCreateCommandPool);                                                  \
  MACRO(vkDestroyCommandPool);                                                 \
  MACRO(vkAllocateCommandBuffers);                                             \
  MACRO(vkFreeCommandBuffers);                                                 \
  MACRO(vkCreateSemaphore);                                                    \
  MACRO(vkDestroySemaphore);                                                   \
  MACRO(vkCreateFence);                                                        \
  MACRO(vkDestroyFence);                                                       \
  MACRO(vkWaitForFences);                                                      \
  MACRO(vkResetFences);                                                        \
  MACRO(vkAcquireNextImageKHR);                                                \
  MACRO(vkQueueSubmit);                                                        \
  MACRO(vkQueuePresentKHR);                                                    \
  MACRO(vkCreateBuffer);                                                       \
  MACRO(vkDestroyBuffer);                                                      \
  MACRO(vkDeviceWaitIdle);                                                     \
  MACRO(vkAllocateMemory);                                                     \
  MACRO(vkFreeMemory);                                                         \
  MACRO(vkBindBufferMemory);                                                   \
  MACRO(vkMapMemory);                                                          \
  MACRO(vkUnmapMemory);                                                        \
  MACRO(vkQueueWaitIdle);                                                      \
  MACRO(vkCreateDescriptorSetLayout);                                          \
  MACRO(vkDestroyDescriptorSetLayout);                                         \
  MACRO(vkCreateDescriptorPool);                                               \
  MACRO(vkDestroyDescriptorPool);                                              \
  MACRO(vkAllocateDescriptorSets);                                             \
  MACRO(vkUpdateDescriptorSets);                                               \
  MACRO(vkCreateImage);                                                        \
  MACRO(vkDestroyImage);                                                       \
  MACRO(vkGetImageMemoryRequirements);                                         \
  MACRO(vkBindImageMemory)

#define MEMBER(name) PFN_##name name
  DEVICE_DEFS(MEMBER);
#undef MEMBER

  void loadFunctions();

private:
  std::shared_ptr<Loader> loader = {};
  VkDevice device = VK_NULL_HANDLE;
};