#pragma once
#include <vkt/device.h>

struct DescriptorSetLayoutCreateInfo {
  VkDescriptorSetLayoutCreateFlags flags;
  std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class DescriptorSetLayout {
public:
  DescriptorSetLayout() = default;
  DescriptorSetLayout(std::shared_ptr<Device> device,
                      DescriptorSetLayoutCreateInfo const &createInfo);

  operator VkDescriptorSetLayout();

private:
  std::shared_ptr<Device> device = {};
  Handle<VkDescriptorSetLayout, Device> descriptorSetLayout;
};