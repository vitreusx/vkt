#include <vkt/command_buffer.h>

CommandBuffer::CommandBuffer(std::shared_ptr<Device> device,
                             CommandBufferAllocateInfo allocInfo) {
  this->device = device;
  this->commandPool = allocInfo.commandPool;

  VkCommandBufferAllocateInfo vk_allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = *allocInfo.commandPool,
      .level = allocInfo.level,
      .commandBufferCount = 1,
  };

  VK_CHECK(
      device->vkAllocateCommandBuffers(*device, &vk_allocInfo, &commandBuffer));

  loadFunctions();
}

CommandBuffer::~CommandBuffer() {
  if (commandBuffer != VK_NULL_HANDLE)
    device->vkFreeCommandBuffers(*device, *commandPool, 1, &commandBuffer);
  commandBuffer = VK_NULL_HANDLE;
}

CommandBuffer::operator VkCommandBuffer() {
  return commandBuffer;
}

void CommandBuffer::reset(VkCommandBufferResetFlags flags) {
  VK_CHECK(vkResetCommandBuffer(commandBuffer, flags));
}

void CommandBuffer::loadFunctions() {
#define LOAD(name) this->name = (PFN_##name)vkGetDeviceProcAddr(*device, #name)
  LOAD(vkBeginCommandBuffer);
  LOAD(vkEndCommandBuffer);
  LOAD(vkCmdBeginRenderPass);
  LOAD(vkCmdEndRenderPass);
  LOAD(vkCmdClearColorImage);
  LOAD(vkCmdBindPipeline);
  LOAD(vkCmdSetViewport);
  LOAD(vkCmdSetScissor);
  LOAD(vkCmdDraw);
  LOAD(vkResetCommandBuffer);
  LOAD(vkCmdPipelineBarrier);
  LOAD(vkCmdBindVertexBuffers);
  LOAD(vkCmdCopyBuffer);
  LOAD(vkCmdDrawIndexed);
  LOAD(vkCmdBindIndexBuffer);
  LOAD(vkCmdBindDescriptorSets);
#undef LOAD
}

CommandBufferRecording::CommandBufferRecording(
    std::shared_ptr<CommandBuffer> commandBuffer,
    CommandBufferBeginInfo const &beginInfo) {
  this->commandBuffer = commandBuffer;

  auto vk_beginInfo = VkCommandBufferBeginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = beginInfo.flags,
      .pInheritanceInfo = nullptr};

  VK_CHECK(commandBuffer->vkBeginCommandBuffer(*commandBuffer, &vk_beginInfo));
}

CommandBufferRecording::~CommandBufferRecording() {
  VK_CHECK(commandBuffer->vkEndCommandBuffer(*commandBuffer));
}

void CommandBufferRecording::clearColorImage(
    VkImage image, VkImageLayout imageLayout, VkClearColorValue const &color,
    std::vector<VkImageSubresourceRange> const &ranges) {
  commandBuffer->vkCmdClearColorImage(*commandBuffer, image, imageLayout,
                                      &color, (uint32_t)ranges.size(),
                                      ranges.data());
}

void CommandBufferRecording::copyBuffer(
    VkBuffer source, VkBuffer dest, std::vector<VkBufferCopy> const &regions) {
  commandBuffer->vkCmdCopyBuffer(*commandBuffer, source, dest,
                                 (uint32_t)regions.size(), regions.data());
}

void CommandBufferRecording::bindDescriptorSets(
    VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
    uint32_t firstSet, std::vector<VkDescriptorSet> const &descriptorSets,
    std::optional<std::vector<uint32_t>> const &dynamicOffsets) {
  uint32_t dynamicOffsetCount = 0;
  uint32_t const *pDynamicOffsets = nullptr;
  if (dynamicOffsets.has_value()) {
    dynamicOffsetCount = dynamicOffsets.value().size();
    pDynamicOffsets = dynamicOffsets.value().data();
  }
  commandBuffer->vkCmdBindDescriptorSets(
      *commandBuffer, pipelineBindPoint, layout, firstSet,
      (uint32_t)descriptorSets.size(), descriptorSets.data(),
      dynamicOffsetCount, pDynamicOffsets);
}

CommandBufferRenderPass::CommandBufferRenderPass(
    std::shared_ptr<CommandBufferRecording> recording,
    RenderPassBeginInfo const &renderPassInfo) {
  this->commandBuffer = recording->commandBuffer;
  boundRefs.push_back(renderPassInfo.renderPass);
  boundRefs.push_back(recording);
  boundRefs.push_back(renderPassInfo.framebuffer);

  auto vk_renderPassInfo = VkRenderPassBeginInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = *renderPassInfo.renderPass,
      .framebuffer = *renderPassInfo.framebuffer,
      .renderArea = renderPassInfo.renderArea,
      .clearValueCount = (uint32_t)renderPassInfo.clearValues.size(),
      .pClearValues = renderPassInfo.clearValues.data()

  };

  commandBuffer->vkCmdBeginRenderPass(*commandBuffer, &vk_renderPassInfo,
                                      renderPassInfo.subpassContents);
}

CommandBufferRenderPass::~CommandBufferRenderPass() {
  commandBuffer->vkCmdEndRenderPass(*commandBuffer);
}

void CommandBufferRenderPass::bindPipeline(
    std::shared_ptr<GraphicsPipeline> pipeline) {
  boundRefs.push_back(pipeline);
  commandBuffer->vkCmdBindPipeline(*commandBuffer,
                                   VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
}

void CommandBufferRenderPass::setViewport(VkViewport const &viewport) {
  commandBuffer->vkCmdSetViewport(*commandBuffer, 0, 1, &viewport);
}

void CommandBufferRenderPass::setScissor(VkRect2D const &scissor) {
  commandBuffer->vkCmdSetScissor(*commandBuffer, 0, 1, &scissor);
}

void CommandBufferRenderPass::draw(uint32_t vertexCount, uint32_t instanceCount,
                                   uint32_t firstVertex,
                                   uint32_t firstInstance) {
  commandBuffer->vkCmdDraw(*commandBuffer, vertexCount, instanceCount,
                           firstVertex, firstInstance);
}

void CommandBufferRenderPass::bindVertexBuffers(
    std::vector<std::pair<std::shared_ptr<Buffer>, VkDeviceSize>> const
        &buffersAndOffsets) {
  std::vector<VkBuffer> vk_buffers;
  std::vector<VkDeviceSize> vk_offsets;
  for (auto const &[buffer, offset] : buffersAndOffsets) {
    vk_buffers.push_back(*buffer);
    vk_offsets.push_back(offset);
  };

  commandBuffer->vkCmdBindVertexBuffers(*commandBuffer, 0,
                                        buffersAndOffsets.size(),
                                        vk_buffers.data(), vk_offsets.data());
}

void CommandBufferRenderPass::drawIndexed(uint32_t indexCount,
                                          uint32_t instanceCount,
                                          uint32_t firstIndex,
                                          int32_t vertexOffset,
                                          uint32_t firstInstance) {
  commandBuffer->vkCmdDrawIndexed(*commandBuffer, indexCount, instanceCount,
                                  firstIndex, vertexOffset, firstInstance);
}

void CommandBufferRenderPass::bindIndexBuffer(VkBuffer buffer,
                                              VkDeviceSize offset,
                                              VkIndexType indexType) {
  commandBuffer->vkCmdBindIndexBuffer(*commandBuffer, buffer, offset,
                                      indexType);
}