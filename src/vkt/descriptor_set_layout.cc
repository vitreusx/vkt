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

  VkDescriptorSetLayout descriptorSetLayout;
  VK_CHECK(device->vkCreateDescriptorSetLayout(*device, &vk_createInfo, nullptr,
                                               &descriptorSetLayout));

  this->descriptorSetLayout = Handle<VkDescriptorSetLayout, Device>(
      descriptorSetLayout,
      [](VkDescriptorSetLayout descriptorSetLayout, Device &device) -> void {
        device.vkDestroyDescriptorSetLayout(device, descriptorSetLayout,
                                            nullptr);
      },
      device);
}

DescriptorSetLayout::operator VkDescriptorSetLayout() {
  return descriptorSetLayout;
}