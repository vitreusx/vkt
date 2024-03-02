#include <vkx/auto_desc_pool.h>

AutoDescriptorPool::AutoDescriptorPool(std::shared_ptr<Device> device)
    : device{std::move(device)} {}

void AutoDescriptorPool::registerType(
    uint32_t typeId,
    std::vector<VkDescriptorSetLayoutBinding> const &bindings) {
  std::map<VkDescriptorType, uint32_t> poolSizes;
  for (auto const &binding : bindings) {
    if (poolSizes.find(binding.descriptorType) == poolSizes.end())
      poolSizes[binding.descriptorType] = 0;
    poolSizes[binding.descriptorType] += binding.descriptorCount;
  }

  types[typeId] = DescriptorSetType{
      .setLayout = DescriptorSetLayout(device, {.bindings = bindings}),
      .poolSizes = poolSizes,
      .pools = {},
      .remSets = 0,
      .maxSets = 8};
}

VkDescriptorSet AutoDescriptorPool::create(uint32_t type) {
  return create(type, 1)[0];
}

std::vector<VkDescriptorSet> AutoDescriptorPool::create(uint32_t typeId,
                                                        uint32_t count) {
  auto &type = types[typeId];
  std::vector<VkDescriptorSet> sets;

  while (count > 0) {
    if (type.remSets == 0) {
      DescriptorPoolCreateInfo createInfo;
      createInfo.maxSets = type.maxSets;
      for (auto const &[descriptorType, totalDescriptorCount] : type.poolSizes)
        createInfo.poolSizes.push_back(
            {.type = descriptorType, .descriptorCount = totalDescriptorCount});

      type.pools.emplace_back(device, createInfo);
      type.remSets = type.maxSets;
      type.maxSets *= 2;
    }

    auto &pool = type.pools.back();
    auto batchSize = std::min(count, type.remSets);
    auto batchSets = pool.allocateDescriptorSets(
        {.setLayouts =
             std::vector<VkDescriptorSetLayout>(batchSize, type.setLayout)});
    sets.insert(sets.end(), batchSets.begin(), batchSets.end());
    count -= batchSize;
    type.remSets -= batchSize;
  }

  return sets;
}