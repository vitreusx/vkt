#pragma once
#include <vkt/device.h>

class Sampler {
public:
  Sampler() = default;
  Sampler(std::shared_ptr<Device> device, VkSamplerCreateInfo createInfo);

  Sampler(Sampler const &) = delete;
  Sampler &operator=(Sampler const &) = delete;

  Sampler(Sampler &&other);
  Sampler &operator=(Sampler &&other);

  ~Sampler();

  operator VkSampler();

private:
  std::shared_ptr<Device> device = {};
  VkSampler sampler = VK_NULL_HANDLE;

  void destroy();
};