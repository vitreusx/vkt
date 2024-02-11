#pragma once
#include <vkt/device.h>

struct DescriptorSetLayoutCreateInfo {
  VkDescriptorSetLayoutCreateFlags flags;
  std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class DescriptorSetLayout {
public:
  DescriptorSetLayout(std::shared_ptr<Device> device,
                      DescriptorSetLayoutCreateInfo const &createInfo);

  DescriptorSetLayout(DescriptorSetLayout const &) = delete;
  DescriptorSetLayout &operator=(DescriptorSetLayout const &) = delete;

  DescriptorSetLayout(DescriptorSetLayout &&);
  DescriptorSetLayout &operator=(DescriptorSetLayout &&);

  ~DescriptorSetLayout();

  operator VkDescriptorSetLayout();

private:
  std::shared_ptr<Device> device = {};
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

  void destroy();
};