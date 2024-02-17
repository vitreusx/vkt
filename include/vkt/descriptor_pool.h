#pragma once
#include <vkt/device.h>
#include <vkt/descriptor_set_layout.h>

struct DescriptorPoolCreateInfo {
  uint32_t maxSets;
  std::vector<VkDescriptorPoolSize> poolSizes;
};

struct DescriptorSetAllocateInfo {
  std::vector<VkDescriptorSetLayout> setLayouts;
};

class DescriptorPool {
public:
  DescriptorPool() = default;
  DescriptorPool(std::shared_ptr<Device> device,
                 DescriptorPoolCreateInfo const &createInfo);

  operator VkDescriptorPool();

  std::vector<VkDescriptorSet>
  allocateDescriptorSets(DescriptorSetAllocateInfo const &allocInfo);

private:
  std::shared_ptr<Device> device = {};
  Handle<VkDescriptorPool, Device> descriptorPool;
};