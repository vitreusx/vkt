#include <vkt/sampler.h>

Sampler::Sampler(std::shared_ptr<Device> device,
                 VkSamplerCreateInfo createInfo) {
  this->device = device;

  createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  createInfo.pNext = nullptr;

  VK_CHECK(device->vkCreateSampler(*device, &createInfo, nullptr, &sampler));
}

Sampler::Sampler(Sampler &&other) {
  *this = std::move(other);
}

Sampler &Sampler::operator=(Sampler &&other) {
  destroy();
  device = std::move(other.device);
  sampler = other.sampler;
  other.sampler = VK_NULL_HANDLE;
  return *this;
}

Sampler::~Sampler() {
  destroy();
}

Sampler::operator VkSampler() {
  return sampler;
}

void Sampler::destroy() {
  if (sampler != VK_NULL_HANDLE)
    device->vkDestroySampler(*device, sampler, nullptr);
}