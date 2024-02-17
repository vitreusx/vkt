#pragma once
#include <vkt/device.h>

class Sampler {
public:
  Sampler() = default;
  Sampler(std::shared_ptr<Device> device, VkSamplerCreateInfo createInfo);

  operator VkSampler();

private:
  std::shared_ptr<Device> device = {};
  Handle<VkSampler, Device> sampler;
};