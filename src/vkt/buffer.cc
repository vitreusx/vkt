#include <vkt/buffer.h>
#include <vkt/command_buffer.h>
#include <vkt/device_memory.h>
#include <cstring>

Buffer::Buffer(std::shared_ptr<Device> device,
               BufferCreateInfo const &createInfo) {
  this->device = device;

  std::vector<uint32_t> queueFamilyIndices;
  for (auto const &queueFamilyIndex : createInfo.queueFamilyIndices)
    queueFamilyIndices.push_back(queueFamilyIndex);

  auto sharingMode = createInfo.sharingMode;
  if (queueFamilyIndices.size() < 2)
    sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBufferCreateInfo vk_createInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = {},
      .size = createInfo.size,
      .usage = createInfo.usage,
      .sharingMode = sharingMode,
      .queueFamilyIndexCount = (uint32_t)queueFamilyIndices.size(),
      .pQueueFamilyIndices = queueFamilyIndices.data()};

  VK_CHECK(device->vkCreateBuffer(*device, &vk_createInfo, nullptr, &buffer));
}

Buffer::Buffer(Buffer &&other) {
  *this = std::move(other);
}

Buffer &Buffer::operator=(Buffer &&other) {
  destroy();
  device = other.device;
  buffer = other.buffer;
  other.buffer = VK_NULL_HANDLE;
  return *this;
}

Buffer::~Buffer() {
  destroy();
}

void Buffer::destroy() {
  if (buffer != VK_NULL_HANDLE)
    device->vkDestroyBuffer(*device, buffer, nullptr);
  buffer = VK_NULL_HANDLE;
}

VkMemoryRequirements Buffer::getMemoryRequirements() {
  VkMemoryRequirements memRequirements = {};
  device->vkGetBufferMemoryRequirements(*device, buffer, &memRequirements);
  return memRequirements;
}

Buffer::operator VkBuffer() {
  return buffer;
}

void Buffer::stage(void *data, VkDeviceSize size, Queue &transferQueue) {
  auto queueFamilyIndex = transferQueue.getQueueFamilyIndex();

  auto staging = Buffer(device, {.size = size,
                                 .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                 .queueFamilyIndices = {queueFamilyIndex}});

  auto stagingMemory =
      staging.allocMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  {
    auto stagingMemoryMap = DeviceMemoryMap(stack_ptr(stagingMemory));
    std::memcpy(stagingMemoryMap.get(), data, size);
  }

  auto cmdPool = CommandPool(
      device,
      CommandPoolCreateInfo{.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                            .queueFamilyIndex = queueFamilyIndex});

  auto copyCmdBuf = CommandBuffer(
      device,
      CommandBufferAllocateInfo{.commandPool = stack_ptr(cmdPool),
                                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY});

  {
    auto rec = CommandBufferRecording(
        stack_ptr(copyCmdBuf),
        CommandBufferBeginInfo{
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT});

    rec.copyBuffer(staging, buffer, {VkBufferCopy{.size = size}});
  }

  transferQueue.submit(QueueSubmitInfo{
      .waitSemaphoresAndStages = {},
      .commandBuffers = {copyCmdBuf},
      .signalSemaphores = {},
      .fence = VK_NULL_HANDLE,
  });
  transferQueue.wait();
}

DeviceMemory Buffer::allocMemory(VkMemoryPropertyFlags properties) {
  auto memoryReqs = getMemoryRequirements();
  auto memoryTypeIndex = device->physDev.findMemoryTypeIndex(
      memoryReqs.memoryTypeBits, properties);
  auto memory = DeviceMemory(
      device, MemoryAllocateInfo{.size = memoryReqs.size,
                                 .memoryTypeIndex = memoryTypeIndex.value()});
  VK_CHECK(device->vkBindBufferMemory(*device, buffer, memory, 0));
  return memory;
}