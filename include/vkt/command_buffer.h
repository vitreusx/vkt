#pragma once
#include <vkt/command_pool.h>
#include <vkt/device.h>
#include <vkt/render_pass.h>
#include <vkt/framebuffer.h>
#include <vkt/graphics_pipeline.h>
#include <vkt/buffer.h>

struct CommandBufferAllocateInfo {
  std::shared_ptr<CommandPool> commandPool;
  VkCommandBufferLevel level;
};

class CommandBuffer {
public:
  CommandBuffer(std::shared_ptr<Device> device,
                CommandBufferAllocateInfo allocInfo);

  CommandBuffer(CommandBuffer const &) = delete;
  CommandBuffer &operator=(CommandBuffer const &) = delete;

  CommandBuffer(CommandBuffer &&) = delete;
  CommandBuffer &operator=(CommandBuffer &&) = delete;

  ~CommandBuffer();

  operator VkCommandBuffer();

  void reset(VkCommandBufferResetFlags flags = {});

#define MEMBER(name) PFN_##name name
  MEMBER(vkBeginCommandBuffer);
  MEMBER(vkEndCommandBuffer);
  MEMBER(vkCmdBeginRenderPass);
  MEMBER(vkCmdEndRenderPass);
  MEMBER(vkCmdClearColorImage);
  MEMBER(vkCmdBindPipeline);
  MEMBER(vkCmdSetViewport);
  MEMBER(vkCmdSetScissor);
  MEMBER(vkCmdDraw);
  MEMBER(vkResetCommandBuffer);
  MEMBER(vkCmdPipelineBarrier);
  MEMBER(vkCmdBindVertexBuffers);
  MEMBER(vkCmdCopyBuffer);
#undef MEMBER

private:
  std::shared_ptr<Device> device;
  std::shared_ptr<CommandPool> commandPool;
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

  void loadFunctions();
};

struct CommandBufferBeginInfo {
  VkCommandBufferUsageFlags flags;
};

class CommandBufferRecording {
public:
  CommandBufferRecording(std::shared_ptr<CommandBuffer> commandBuffer,
                         CommandBufferBeginInfo const &beginInfo);

  CommandBufferRecording(CommandBufferRecording const &) = delete;
  CommandBufferRecording(CommandBufferRecording &&) = delete;

  ~CommandBufferRecording();

  void clearColorImage(VkImage image, VkImageLayout imageLayout,
                       VkClearColorValue const &color,
                       std::vector<VkImageSubresourceRange> const &ranges);

  void copyBuffer(VkBuffer source, VkBuffer dest,
                  std::vector<VkBufferCopy> const &regions);

public:
  std::shared_ptr<CommandBuffer> commandBuffer;
};

struct RenderPassBeginInfo {
  std::shared_ptr<RenderPass> renderPass;
  std::shared_ptr<Framebuffer> framebuffer;
  VkRect2D renderArea;
  std::vector<VkClearValue> clearValues;
  VkSubpassContents subpassContents;
};

class CommandBufferRenderPass {
public:
  CommandBufferRenderPass(std::shared_ptr<CommandBufferRecording> recording,
                          RenderPassBeginInfo const &renderPassInfo);

  CommandBufferRenderPass(CommandBufferRenderPass const &) = delete;
  CommandBufferRenderPass(CommandBufferRenderPass &&) = delete;

  ~CommandBufferRenderPass();

  void bindPipeline(std::shared_ptr<GraphicsPipeline> pipeline);

  void bindVertexBuffers(
      std::vector<std::pair<std::shared_ptr<Buffer>, VkDeviceSize>> const
          &buffersAndOffsets);

  void setViewport(VkViewport const &viewport);

  void setScissor(VkRect2D const &scissor);

  void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
            uint32_t firstInstance);

private:
  std::shared_ptr<CommandBuffer> commandBuffer;
  std::vector<std::shared_ptr<void>> boundRefs;
};