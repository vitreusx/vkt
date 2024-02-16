#pragma once
#include <vkt/instance.h>
#include <variant>

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

struct WriteDescriptorSet {
  VkDescriptorSet dstSet;
  uint32_t dstBinding;
  uint32_t dstArrayElement;
  VkDescriptorType descriptorType;
  std::vector<VkDescriptorImageInfo> imageInfos;
  std::vector<VkDescriptorBufferInfo> bufferInfos;
  std::vector<VkBufferView> texelBufferViews;
};

struct CopyDescriptorSet {
  VkDescriptorSet srcSet;
  uint32_t srcBinding;
  uint32_t srcArrayElement;
  VkDescriptorSet dstSet;
  uint32_t dstBinding;
  uint32_t dstArrayElement;
  uint32_t descriptorCount;
};

using DescriptorOp = std::variant<WriteDescriptorSet, CopyDescriptorSet>;

class Device {
public:
  Device(VkDevice &&device);
  Device(std::shared_ptr<Loader> loader, PhysicalDevice physicalDevice,
         DeviceCreateInfo const &deviceCreateInfo);

  Device(Device const &) = delete;
  Device &operator=(Device const &) = delete;

  Device(Device &&) = delete;
  Device &operator=(Device &&) = delete;

  ~Device();

  operator VkDevice();

  void updateDescriptorSets(std::vector<DescriptorOp> operations);

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
  MACRO(vkBindImageMemory);                                                    \
  MACRO(vkCreateSampler);                                                      \
  MACRO(vkDestroySampler);                                                     \
  MACRO(vkGetBufferMemoryRequirements)

#define MEMBER(name) PFN_##name name
  DEVICE_DEFS(MEMBER);
#undef MEMBER

  PhysicalDevice physDev;

private:
  std::shared_ptr<Loader> loader = {};
  VkDevice device = VK_NULL_HANDLE;

  void loadFunctions();
};