#include <vkt/descriptor_pool.h>

DescriptorPool::DescriptorPool(std::shared_ptr<Device> device,
                               DescriptorPoolCreateInfo const &createInfo) {
  this->device = device;

  auto vk_createInfo = VkDescriptorPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = {},
      .maxSets = createInfo.maxSets,
      .poolSizeCount = (uint32_t)createInfo.poolSizes.size(),
      .pPoolSizes = createInfo.poolSizes.data()};

  VkDescriptorPool descriptorPool;
  VK_CHECK(device->vkCreateDescriptorPool(*device, &vk_createInfo, nullptr,
                                          &descriptorPool));

  this->descriptorPool = Handle<VkDescriptorPool, Device>(
      descriptorPool,
      [](VkDescriptorPool descriptorPool, Device &device) -> void {
        device.vkDestroyDescriptorPool(device, descriptorPool, nullptr);
      },
      device);
}

DescriptorPool::operator VkDescriptorPool() {
  return descriptorPool;
}

std::vector<VkDescriptorSet> DescriptorPool::allocateDescriptorSets(
    DescriptorSetAllocateInfo const &allocInfo) {

  VkDescriptorSetAllocateInfo vk_allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = descriptorPool,
      .descriptorSetCount = (uint32_t)allocInfo.setLayouts.size(),
      .pSetLayouts = allocInfo.setLayouts.data(),
  };

  std::vector<VkDescriptorSet> sets(vk_allocInfo.descriptorSetCount);
  VK_CHECK(
      device->vkAllocateDescriptorSets(*device, &vk_allocInfo, sets.data()));
  return sets;
}
