#include <vkt/descriptor_set_layout.h>

DescriptorSetLayout::DescriptorSetLayout(
    std::shared_ptr<Device> device,
    DescriptorSetLayoutCreateInfo const &createInfo) {
  this->device = device;

  auto vk_createInfo = VkDescriptorSetLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = createInfo.flags,
      .bindingCount = (uint32_t)createInfo.bindings.size(),
      .pBindings = createInfo.bindings.data()};

  VK_CHECK(device->vkCreateDescriptorSetLayout(*device, &vk_createInfo, nullptr,
                                               &descriptorSetLayout));
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout &&other) {
  *this = std::move(other);
}

DescriptorSetLayout &
DescriptorSetLayout::operator=(DescriptorSetLayout &&other) {
  destroy();
  device = std::move(other.device);
  descriptorSetLayout = other.descriptorSetLayout;
  other.descriptorSetLayout = VK_NULL_HANDLE;
  return *this;
}

DescriptorSetLayout::~DescriptorSetLayout() {
  destroy();
}

void DescriptorSetLayout::destroy() {
  if (descriptorSetLayout != VK_NULL_HANDLE)
    device->vkDestroyDescriptorSetLayout(*device, descriptorSetLayout, nullptr);
  descriptorSetLayout = VK_NULL_HANDLE;
}

DescriptorSetLayout::operator VkDescriptorSetLayout() {
  return descriptorSetLayout;
}