#pragma once
#include <vkt/device.h>
#include <vkt/render_pass.h>
#include <vkt/image_view.h>

struct FramebufferCreateInfo {
  std::vector<std::shared_ptr<ImageView>> attachments;
  std::shared_ptr<RenderPass> renderPass;
  VkExtent2D extent;
  uint32_t layers;
};

class Framebuffer {
public:
  Framebuffer() = default;
  Framebuffer(std::shared_ptr<Device> device,
              FramebufferCreateInfo const &createInfo);

  operator VkFramebuffer();

private:
  std::shared_ptr<Device> device = {};
  std::shared_ptr<RenderPass> renderPass = {};
  Handle<VkFramebuffer, Device> framebuffer;
  std::vector<std::shared_ptr<ImageView>> attachments = {};
};