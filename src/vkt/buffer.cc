#include <vkt/buffer.h>

Buffer::Buffer(std::shared_ptr<Device> device,
               BufferCreateInfo const &createInfo) {
  this->device = device;

  VkBufferCreateInfo vk_createInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = {},
      .size = createInfo.size,
      .usage = createInfo.usage,
      .sharingMode = createInfo.sharingMode,
      .queueFamilyIndexCount = (uint32_t)createInfo.queueFamilyIndices.size(),
      .pQueueFamilyIndices = createInfo.queueFamilyIndices.data()};

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
  vkGetBufferMemoryRequirements(*device, buffer, &memRequirements);
  return memRequirements;
}

Buffer::operator VkBuffer() {
  return buffer;
}