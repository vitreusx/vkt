#pragma once
#include <vkt/device.h>
#include <map>
#include <vkt/descriptor_set_layout.h>
#include <vkt/descriptor_pool.h>

class AutoDescriptorPool {
public:
  AutoDescriptorPool() = default;
  AutoDescriptorPool(std::shared_ptr<Device> device);

  void registerType(uint32_t typeId,
                    std::vector<VkDescriptorSetLayoutBinding> const &bindings);

  VkDescriptorSet create(uint32_t type);
  std::vector<VkDescriptorSet> create(uint32_t typeId, uint32_t count);

private:
  std::shared_ptr<Device> device;

  struct DescriptorSetType {
    DescriptorSetLayout setLayout;
    std::map<VkDescriptorType, uint32_t> poolSizes;
    std::vector<DescriptorPool> pools;
    uint32_t remSets, maxSets;
  };

  std::map<uint32_t, DescriptorSetType> types;
};