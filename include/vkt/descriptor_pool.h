#pragma once
#include <vkt/device.h>
#include <vkt/descriptor_set_layout.h>
#include <variant>

struct DescriptorPoolCreateInfo {
  uint32_t maxSets;
  std::vector<VkDescriptorPoolSize> poolSizes;
};

struct DescriptorSetAllocateInfo {
  std::vector<std::shared_ptr<DescriptorSetLayout>> setLayouts;
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

class DescriptorPool {
public:
  DescriptorPool(std::shared_ptr<Device> device,
                 DescriptorPoolCreateInfo const &createInfo);

  DescriptorPool(DescriptorPool const &) = delete;
  DescriptorPool &operator=(DescriptorPool const &) = delete;

  DescriptorPool(DescriptorPool &&);
  DescriptorPool &operator=(DescriptorPool &&);

  ~DescriptorPool();

  operator VkDescriptorPool();

  std::vector<VkDescriptorSet>
  allocateDescriptorSets(DescriptorSetAllocateInfo const &allocInfo);

  void updateDescriptorSets(
      std::vector<std::variant<WriteDescriptorSet, CopyDescriptorSet>>
          operations);

private:
  std::shared_ptr<Device> device = {};
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

  void destroy();
};