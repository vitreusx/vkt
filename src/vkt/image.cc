#include <vkt/image.h>
#include <vkt/buffer.h>
#include <vkt/command_buffer.h>
#include <cstring>

Image::Image(std::shared_ptr<Device> device,
             ImageCreateInfo const &createInfo) {
  this->device = device;
  this->createInfo = createInfo;

  std::vector<uint32_t> queueFamilyIndices;
  for (auto queueFamilyIndex : createInfo.queueFamilyIndices)
    queueFamilyIndices.push_back(queueFamilyIndex);

  auto sharingMode = createInfo.sharingMode;
  if (queueFamilyIndices.size() == 1)
    sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  auto vk_createInfo = VkImageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = createInfo.flags,
      .imageType = createInfo.imageType,
      .format = createInfo.format,
      .extent = createInfo.extent,
      .mipLevels = createInfo.mipLevels,
      .arrayLayers = createInfo.arrayLayers,
      .samples = createInfo.samples,
      .tiling = createInfo.tiling,
      .usage = createInfo.usage,
      .sharingMode = sharingMode,
      .queueFamilyIndexCount = (uint32_t)queueFamilyIndices.size(),
      .pQueueFamilyIndices = queueFamilyIndices.data(),
      .initialLayout = createInfo.initialLayout,
  };

  VkImage image;
  VK_CHECK(device->vkCreateImage(*device, &vk_createInfo, nullptr, &image));

  this->image = Handle<VkImage, Device>(
      image,
      [](VkImage image, Device &device) -> void {
        device.vkDestroyImage(device, image, nullptr);
      },
      device);

  vkGetPhysicalDeviceImageFormatProperties(
      device->physDev, createInfo.format, createInfo.imageType,
      createInfo.tiling, createInfo.usage, createInfo.flags, &formatProps);
}

Image::operator VkImage() {
  return image;
}

VkMemoryRequirements Image::getMemoryRequirements() {
  VkMemoryRequirements memRequirements = {};
  device->vkGetImageMemoryRequirements(*device, image, &memRequirements);
  return memRequirements;
}

DeviceMemory Image::allocMemory(VkMemoryPropertyFlags properties) {
  auto memoryReqs = getMemoryRequirements();
  auto memoryTypeIndex = device->physDev.findMemoryTypeIndex(
      memoryReqs.memoryTypeBits, properties);
  auto memory = DeviceMemory(
      device, MemoryAllocateInfo{.size = memoryReqs.size,
                                 .memoryTypeIndex = memoryTypeIndex.value()});
  VK_CHECK(device->vkBindImageMemory(*device, image, memory, 0));
  return memory;
}

void Image::stage(void *data, VkDeviceSize size, Queue &transferQueue,
                  VkPipelineStageFlags dstStageMask,
                  VkAccessFlags dstAccessMask, VkImageLayout dstLayout) {
  auto queueFamilyIndex = transferQueue.getQueueFamilyIndex();

  auto staging = Buffer(
      device, BufferCreateInfo{.size = size,
                               .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                               .queueFamilyIndices = {queueFamilyIndex}});

  auto stagingMemory =
      staging.allocMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  {
    auto stagingMap = DeviceMemoryMap(stack_ptr(stagingMemory));
    std::memcpy(stagingMap.get(), data, size);
  }

  auto cmdPool = CommandPool(
      device,
      CommandPoolCreateInfo{.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                            .queueFamilyIndex = queueFamilyIndex});

  auto cmdBuf = CommandBuffer(
      device,
      CommandBufferAllocateInfo{.commandPool = stack_ptr(cmdPool),
                                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY});

  {
    auto rec = CommandBufferRecording(
        stack_ptr(cmdBuf),
        CommandBufferBeginInfo{
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT});

    auto subresourceRange =
        VkImageSubresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .baseMipLevel = 0,
                                .levelCount = 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1};

    rec.pipelineBarrier(
        DependencyInfo{.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       .dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                       .dependencyFlags = {},
                       .imageMemoryBarriers = {VkImageMemoryBarrier{
                           .srcAccessMask = {},
                           .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                           .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                           .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                           .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                           .image = image,
                           .subresourceRange = subresourceRange}}});

    rec.copyBufferToImage(CopyBufferToImageInfo{
        .srcBuffer = staging,
        .dstImage = image,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regions = {
            VkBufferImageCopy{.bufferOffset = 0,
                              .bufferRowLength = 0,
                              .bufferImageHeight = 0,
                              .imageSubresource =
                                  VkImageSubresourceLayers{
                                      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                      .mipLevel = 0,
                                      .baseArrayLayer = 0,
                                      .layerCount = 1},
                              .imageOffset = {},
                              .imageExtent = createInfo.extent}}});

    rec.pipelineBarrier(
        DependencyInfo{.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                       .dstStageMask = dstStageMask,
                       .dependencyFlags = {},
                       .imageMemoryBarriers = {VkImageMemoryBarrier{
                           .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                           .dstAccessMask = dstAccessMask,
                           .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           .newLayout = dstLayout,
                           .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                           .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                           .image = image,
                           .subresourceRange = subresourceRange}}});
  }

  transferQueue.submit(QueueSubmitInfo{.commandBuffers = {cmdBuf}});
  transferQueue.wait();
}