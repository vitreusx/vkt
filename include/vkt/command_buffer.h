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

  operator VkCommandBuffer();

  void reset(VkCommandBufferResetFlags flags = {});

#define CMD_BUF_DEFS(MACRO)                                                    \
  MACRO(vkBeginCommandBuffer);                                                 \
  MACRO(vkEndCommandBuffer);                                                   \
  MACRO(vkCmdBeginRenderPass);                                                 \
  MACRO(vkCmdEndRenderPass);                                                   \
  MACRO(vkCmdClearColorImage);                                                 \
  MACRO(vkCmdBindPipeline);                                                    \
  MACRO(vkCmdSetViewport);                                                     \
  MACRO(vkCmdSetScissor);                                                      \
  MACRO(vkCmdDraw);                                                            \
  MACRO(vkResetCommandBuffer);                                                 \
  MACRO(vkCmdPipelineBarrier);                                                 \
  MACRO(vkCmdBindVertexBuffers);                                               \
  MACRO(vkCmdCopyBuffer);                                                      \
  MACRO(vkCmdDrawIndexed);                                                     \
  MACRO(vkCmdBindIndexBuffer);                                                 \
  MACRO(vkCmdBindDescriptorSets);                                              \
  MACRO(vkCmdCopyBufferToImage);                                               \
  MACRO(vkCmdNextSubpass)

#define MEMBER(name) PFN_##name name
  CMD_BUF_DEFS(MEMBER);
#undef MEMBER

private:
  std::shared_ptr<Device> device;
  Handle<VkCommandBuffer, Device, CommandPool> commandBuffer = {};

  void loadFunctions();
};

struct CommandBufferBeginInfo {
  VkCommandBufferUsageFlags flags;
};

struct DependencyInfo {
  VkPipelineStageFlags srcStageMask;
  VkPipelineStageFlags dstStageMask;
  VkDependencyFlags dependencyFlags;
  mutable std::vector<VkMemoryBarrier> memoryBarriers;
  mutable std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
  mutable std::vector<VkImageMemoryBarrier> imageMemoryBarriers;
};

struct CopyBufferToImageInfo {
  VkBuffer srcBuffer;
  VkImage dstImage;
  VkImageLayout dstImageLayout;
  std::vector<VkBufferImageCopy> regions;
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

  void bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,
                          VkPipelineLayout layout, uint32_t firstSet,
                          std::vector<VkDescriptorSet> const &descriptorSets,
                          std::optional<std::vector<uint32_t>> const
                              &dynamicOffsets = std::nullopt);

  void pipelineBarrier(DependencyInfo const &depInfo);

  void copyBufferToImage(CopyBufferToImageInfo const &copyInfo);

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

  void bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset,
                       VkIndexType indexType);

  void setViewport(VkViewport const &viewport);

  void setScissor(VkRect2D const &scissor);

  void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
            uint32_t firstInstance);

  void drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                   uint32_t firstIndex, int32_t vertexOffset,
                   uint32_t firstInstance);

  void nextSubpass(VkSubpassContents contents);

private:
  std::shared_ptr<CommandBuffer> commandBuffer;
  std::vector<std::shared_ptr<void>> boundRefs;
};