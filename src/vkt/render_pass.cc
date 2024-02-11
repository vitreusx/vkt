#include <vkt/render_pass.h>

RenderPass::RenderPass(std::shared_ptr<Device> device,
                       RenderPassCreateInfo const &createInfo) {
  this->device = device;

  auto vk_subpasses =
      mapV(createInfo.subpasses, [](SubpassDescription const &subpass) -> auto {
        return VkSubpassDescription{
            .flags = subpass.flags,
            .pipelineBindPoint = subpass.pipelineBindPoint,
            .inputAttachmentCount = (uint32_t)subpass.input.size(),
            .pInputAttachments = subpass.input.data(),
            .colorAttachmentCount = (uint32_t)subpass.color.size(),
            .pColorAttachments = subpass.color.data(),
            .pResolveAttachments = subpass.resolve.data(),
            .pDepthStencilAttachment = &subpass.depthStencil,
            .preserveAttachmentCount = (uint32_t)subpass.preserve.size(),
            .pPreserveAttachments = subpass.preserve.data()};
      });

  auto vk_createInfo = VkRenderPassCreateInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = VK_NULL_HANDLE,
      .flags = {},
      .attachmentCount = (uint32_t)createInfo.attachments.size(),
      .pAttachments = createInfo.attachments.data(),
      .subpassCount = (uint32_t)vk_subpasses.size(),
      .pSubpasses = vk_subpasses.data(),
      .dependencyCount = (uint32_t)createInfo.dependencies.size(),
      .pDependencies = createInfo.dependencies.data()};

  VK_CHECK(device->vkCreateRenderPass(*device, &vk_createInfo, VK_NULL_HANDLE,
                                      &renderPass));
}

RenderPass::RenderPass(RenderPass &&other) {
  *this = std::move(other);
}

RenderPass &RenderPass::operator=(RenderPass &&other) {
  destroy();
  device = std::move(other.device);
  renderPass = other.renderPass;
  other.renderPass = VK_NULL_HANDLE;
  return *this;
}

RenderPass::~RenderPass() {
  destroy();
}

void RenderPass::destroy() {
  if (renderPass != VK_NULL_HANDLE)
    device->vkDestroyRenderPass(*device, renderPass, VK_NULL_HANDLE);
  renderPass = VK_NULL_HANDLE;
}

RenderPass::operator VkRenderPass() {
  return renderPass;
}