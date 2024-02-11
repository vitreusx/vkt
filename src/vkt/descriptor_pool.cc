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

  VK_CHECK(device->vkCreateDescriptorPool(*device, &vk_createInfo, nullptr,
                                          &descriptorPool));
}

DescriptorPool::DescriptorPool(DescriptorPool &&other) {
  *this = std::move(other);
}

DescriptorPool &DescriptorPool::operator=(DescriptorPool &&other) {
  destroy();
  device = std::move(other.device);
  descriptorPool = other.descriptorPool;
  other.descriptorPool = VK_NULL_HANDLE;
  return *this;
}

DescriptorPool::~DescriptorPool() {
  destroy();
}

void DescriptorPool::destroy() {
  if (descriptorPool != VK_NULL_HANDLE)
    device->vkDestroyDescriptorPool(*device, descriptorPool, nullptr);
  descriptorPool = VK_NULL_HANDLE;
}

DescriptorPool::operator VkDescriptorPool() {
  return descriptorPool;
}

std::vector<VkDescriptorSet> DescriptorPool::allocateDescriptorSets(
    DescriptorSetAllocateInfo const &allocInfo) {

  std::vector<VkDescriptorSetLayout> setLayouts;
  for (auto const &setLayout : allocInfo.setLayouts)
    setLayouts.push_back(*setLayout);

  VkDescriptorSetAllocateInfo vk_allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = descriptorPool,
      .descriptorSetCount = (uint32_t)setLayouts.size(),
      .pSetLayouts = setLayouts.data(),
  };

  std::vector<VkDescriptorSet> sets(vk_allocInfo.descriptorSetCount);
  VK_CHECK(
      device->vkAllocateDescriptorSets(*device, &vk_allocInfo, sets.data()));
  return sets;
}

void DescriptorPool::updateDescriptorSets(
    std::vector<std::variant<WriteDescriptorSet, CopyDescriptorSet>>
        operations) {
  std::vector<VkWriteDescriptorSet> writeOps;
  std::vector<VkCopyDescriptorSet> copyOps;
  for (auto &op : operations) {
    if (std::holds_alternative<WriteDescriptorSet>(op)) {
      auto &writeOp = std::get<WriteDescriptorSet>(op);

      auto vk_writeOp =
          VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .pNext = nullptr,
                               .dstSet = writeOp.dstSet,
                               .dstBinding = writeOp.dstBinding,
                               .dstArrayElement = writeOp.dstArrayElement,
                               .descriptorType = writeOp.descriptorType};

      if (!writeOp.imageInfos.empty()) {
        vk_writeOp.descriptorCount = writeOp.imageInfos.size();
        vk_writeOp.pImageInfo = writeOp.imageInfos.data();
      } else if (!writeOp.bufferInfos.empty()) {
        vk_writeOp.descriptorCount = writeOp.bufferInfos.size();
        vk_writeOp.pBufferInfo = writeOp.bufferInfos.data();
      } else if (!writeOp.texelBufferViews.empty()) {
        vk_writeOp.descriptorCount = writeOp.texelBufferViews.size();
        vk_writeOp.pTexelBufferView = writeOp.texelBufferViews.data();
      }

      writeOps.push_back(std::move(vk_writeOp));
    } else if (std::holds_alternative<CopyDescriptorSet>(op)) {
      auto const &copyOp = std::get<CopyDescriptorSet>(op);
      copyOps.push_back(
          VkCopyDescriptorSet{.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
                              .pNext = nullptr,
                              .srcSet = copyOp.srcSet,
                              .srcBinding = copyOp.srcBinding,
                              .srcArrayElement = copyOp.srcArrayElement,
                              .dstSet = copyOp.dstSet,
                              .dstBinding = copyOp.dstBinding,
                              .dstArrayElement = copyOp.dstArrayElement,
                              .descriptorCount = copyOp.descriptorCount});
    }
  }

  device->vkUpdateDescriptorSets(*device, (uint32_t)writeOps.size(),
                                 writeOps.data(), (uint32_t)copyOps.size(),
                                 copyOps.data());
}