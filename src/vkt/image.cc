#include <vkt/image.h>

Image::Image(std::shared_ptr<Device> device,
             ImageCreateInfo const &createInfo) {
  this->device = device;

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
      .sharingMode = createInfo.sharingMode,
      .queueFamilyIndexCount = (uint32_t)createInfo.queueFamilyIndices.size(),
      .pQueueFamilyIndices = createInfo.queueFamilyIndices.data(),
      .initialLayout = createInfo.initialLayout,
  };

  VK_CHECK(device->vkCreateImage(*device, &vk_createInfo, nullptr, &image));
}

Image::Image(Image &&other) {
  *this = std::move(other);
}

Image &Image::operator=(Image &&other) {
  destroy();
  device = std::move(other.device);
  image = other.image;
  other.image = VK_NULL_HANDLE;
  return *this;
}

Image::~Image() {
  destroy();
}

void Image::destroy() {
  if (image != VK_NULL_HANDLE)
    device->vkDestroyImage(*device, image, nullptr);
  image = VK_NULL_HANDLE;
}

Image::operator VkImage() {
  return image;
}
