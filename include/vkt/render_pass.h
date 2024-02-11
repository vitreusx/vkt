#pragma once
#include <vkt/device.h>

struct SubpassDescription {
  VkSubpassDescriptionFlags flags;
  VkPipelineBindPoint pipelineBindPoint;
  VkAttachmentReference depthStencil;
  std::vector<VkAttachmentReference> input, color, resolve;
  std::vector<uint32_t> preserve;
};

struct RenderPassCreateInfo {
  std::vector<VkAttachmentDescription> attachments;
  std::vector<SubpassDescription> subpasses;
  std::vector<VkSubpassDependency> dependencies;
};

class RenderPass {
public:
  RenderPass() = default;
  RenderPass(std::shared_ptr<Device> device,
             RenderPassCreateInfo const &createInfo);

  RenderPass(RenderPass const &) = delete;
  RenderPass &operator=(RenderPass const &) = delete;

  RenderPass(RenderPass &&other);
  RenderPass &operator=(RenderPass &&other);

  ~RenderPass();

  operator VkRenderPass();

private:
  std::shared_ptr<Device> device = {};
  VkRenderPass renderPass = VK_NULL_HANDLE;

  void destroy();
};