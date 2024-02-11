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

  VK_CHECK(device->vkCreateFramebuffer(*device, &vk_createInfo, nullptr,
                                       &framebuffer));
}

Framebuffer::Framebuffer(Framebuffer &&other) {
  *this = std::move(other);
}

Framebuffer &Framebuffer::operator=(Framebuffer &&other) {
  destroy();
  device = std::move(other.device);
  renderPass = std::move(other.renderPass);
  attachments = std::move(other.attachments);
  framebuffer = other.framebuffer;
  other.framebuffer = VK_NULL_HANDLE;
  return *this;
}

Framebuffer::~Framebuffer() {
  destroy();
}

void Framebuffer::destroy() {
  if (framebuffer != VK_NULL_HANDLE)
    device->vkDestroyFramebuffer(*device, framebuffer, nullptr);
  framebuffer = VK_NULL_HANDLE;
}

Framebuffer::operator VkFramebuffer() {
  return framebuffer;
}