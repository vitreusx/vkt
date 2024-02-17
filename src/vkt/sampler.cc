#include <vkt/sampler.h>

Sampler::Sampler(std::shared_ptr<Device> device,
                 VkSamplerCreateInfo createInfo) {
  this->device = device;

  createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  createInfo.pNext = nullptr;

  VkSampler sampler;
  VK_CHECK(device->vkCreateSampler(*device, &createInfo, nullptr, &sampler));

  this->sampler = Handle<VkSampler, Device>(
      sampler,
      [](VkSampler sampler, Device &device) -> void {
        device.vkDestroySampler(device, sampler, nullptr);
      },
      device);
}

Sampler::operator VkSampler() {
  return sampler;
}
