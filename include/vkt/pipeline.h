#pragma once
#include <vkt/device.h>

class Pipeline {
public:
  Pipeline() = default;
  Pipeline(std::shared_ptr<Device> device, VkPipeline &&pipeline);

  operator VkPipeline();

protected:
  std::shared_ptr<Device> device = {};
  Handle<VkPipeline, Device> pipeline;
};