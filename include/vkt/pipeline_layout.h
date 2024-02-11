#pragma once
#include <vkt/device.h>

struct PipelineLayoutCreateInfo {
  std::vector<VkDescriptorSetLayout> setLayouts;
  std::vector<VkPushConstantRange> pushConstantRanges;
};

class PipelineLayout {
public:
  PipelineLayout(std::shared_ptr<Device> device,
                 PipelineLayoutCreateInfo createInfo);

  PipelineLayout(PipelineLayout const &) = delete;
  PipelineLayout &operator=(PipelineLayout const &) = delete;

  PipelineLayout(PipelineLayout &&other);

  PipelineLayout &operator=(PipelineLayout &&other);

  ~PipelineLayout();

  void destroy();

  operator VkPipelineLayout();

private:
  std::shared_ptr<Device> device = {};
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};