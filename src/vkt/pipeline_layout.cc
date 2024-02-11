#include <vkt/pipeline_layout.h>

PipelineLayout::PipelineLayout(std::shared_ptr<Device> device,
                               PipelineLayoutCreateInfo createInfo) {
  this->device = device;
  VkPipelineLayoutCreateInfo vk_createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = VK_NULL_HANDLE,
      .flags = {},
      .setLayoutCount = (uint32_t)createInfo.setLayouts.size(),
      .pSetLayouts = createInfo.setLayouts.data(),
      .pushConstantRangeCount = (uint32_t)createInfo.pushConstantRanges.size(),
      .pPushConstantRanges = createInfo.pushConstantRanges.data()};

  VK_CHECK(device->vkCreatePipelineLayout(*device, &vk_createInfo,
                                          VK_NULL_HANDLE, &pipelineLayout));
}

PipelineLayout::PipelineLayout(PipelineLayout &&other) {
  *this = std::move(other);
}

PipelineLayout &PipelineLayout::operator=(PipelineLayout &&other) {
  destroy();
  device = std::move(other.device);
  pipelineLayout = other.pipelineLayout;
  other.pipelineLayout = VK_NULL_HANDLE;
  return *this;
}

PipelineLayout::~PipelineLayout() {
  destroy();
}

void PipelineLayout::destroy() {
  if (pipelineLayout != VK_NULL_HANDLE)
    device->vkDestroyPipelineLayout(*device, pipelineLayout, VK_NULL_HANDLE);
  pipelineLayout = VK_NULL_HANDLE;
}

PipelineLayout::operator VkPipelineLayout() {
  return pipelineLayout;
}
