#include <vkt/buffer.h>

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
  vkGetBufferMemoryRequirements(*device, buffer, &memRequirements);
  return memRequirements;
}

Buffer::operator VkBuffer() {
  return buffer;
}