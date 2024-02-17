#include <vkt/framebuffer.h>

Framebuffer::Framebuffer(std::shared_ptr<Device> device,
                         FramebufferCreateInfo const &createInfo) {
  this->device = device;
  this->renderPass = createInfo.renderPass;
  this->attachments = createInfo.attachments;

  std::vector<VkImageView> vk_attachments =
      mapV(createInfo.attachments, [](auto const &imageView) -> auto {
        return (VkImageView)*imageView;
      });

  auto vk_createInfo = VkFramebufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = {},
      .renderPass = *createInfo.renderPass,
      .attachmentCount = (uint32_t)createInfo.attachments.size(),
      .pAttachments = vk_attachments.data(),
      .width = createInfo.extent.width,
      .height = createInfo.extent.height,
      .layers = createInfo.layers};

  VkFramebuffer framebuffer;
  VK_CHECK(device->vkCreateFramebuffer(*device, &vk_createInfo, nullptr,
                                       &framebuffer));

  this->framebuffer = Handle<VkFramebuffer, Device>(
      framebuffer,
      [](VkFramebuffer framebuffer, Device &device) -> void {
        device.vkDestroyFramebuffer(device, framebuffer, nullptr);
      },
      device);
}

Framebuffer::operator VkFramebuffer() {
  return framebuffer;
}