#pragma once
#include <vkt/device.h>

struct PipelineLayoutCreateInfo {
  std::vector<VkDescriptorSetLayout> setLayouts;
  std::vector<VkPushConstantRange> pushConstantRanges;
};

class PipelineLayout {
public:
  PipelineLayout() = default;
  PipelineLayout(std::shared_ptr<Device> device,
                 PipelineLayoutCreateInfo createInfo);

  operator VkPipelineLayout();

private:
  std::shared_ptr<Device> device = {};
  Handle<VkPipelineLayout, Device> pipelineLayout;
};