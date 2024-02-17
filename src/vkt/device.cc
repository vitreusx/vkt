#include <vkt/device.h>

Device::Device(std::shared_ptr<Loader> loader, PhysicalDevice physicalDevice,
               DeviceCreateInfo const &deviceCreateInfo) {
  this->loader = loader;
  this->physDev = physicalDevice;

  auto vk_queueCreateInfos =
      mapV(deviceCreateInfo.queueCreateInfos,
           [](auto const &queueCreateInfo) -> auto {
             return VkDeviceQueueCreateInfo{
                 .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                 .pNext = VK_NULL_HANDLE,
                 .flags = queueCreateInfo.flags,
                 .queueFamilyIndex = queueCreateInfo.queueFamilyIndex,
                 .queueCount = (uint32_t)queueCreateInfo.queuePriorities.size(),
                 .pQueuePriorities = queueCreateInfo.queuePriorities.data()};
           });

  auto extNames = vkMapNames(deviceCreateInfo.enabledExtensions);
  auto layerNames = vkMapNames(deviceCreateInfo.enabledLayers);

  VkDeviceCreateInfo vk_deviceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = VK_NULL_HANDLE,
      .flags = deviceCreateInfo.flags,
      .queueCreateInfoCount = (uint32_t)vk_queueCreateInfos.size(),
      .pQueueCreateInfos = vk_queueCreateInfos.data(),
      .enabledLayerCount = (uint32_t)layerNames.size(),
      .ppEnabledLayerNames = layerNames.data(),
      .enabledExtensionCount = (uint32_t)extNames.size(),
      .ppEnabledExtensionNames = extNames.data(),
      .pEnabledFeatures = &deviceCreateInfo.enabledFeatures};

  VkDevice device;
  VK_CHECK(loader->vkCreateDevice(physicalDevice, &vk_deviceCreateInfo, nullptr,
                                  &device));

  this->device = Handle<VkDevice, Loader>(
      device,
      [](VkDevice device, Loader &loader) -> void {
        loader.vkDestroyDevice(device, nullptr);
      },
      loader);

  loadFunctions();
}

Device::operator VkDevice() {
  return device;
}

void Device::updateDescriptorSets(
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

  vkUpdateDescriptorSets(device, (uint32_t)writeOps.size(), writeOps.data(),
                         (uint32_t)copyOps.size(), copyOps.data());
}

void Device::loadFunctions() {
#define LOAD(name) this->name = (PFN_##name)vkGetDeviceProcAddr(device, #name)
  DEVICE_DEFS(LOAD);
#undef LOAD
}