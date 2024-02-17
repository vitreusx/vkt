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

  VkPipelineLayout pipelineLayout;
  VK_CHECK(device->vkCreatePipelineLayout(*device, &vk_createInfo, nullptr,
                                          &pipelineLayout));

  this->pipelineLayout = Handle<VkPipelineLayout, Device>(
      pipelineLayout,
      [](VkPipelineLayout pipelineLayout, Device &device) -> void {
        device.vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
      },
      device);
}

PipelineLayout::operator VkPipelineLayout() {
  return pipelineLayout;
}
