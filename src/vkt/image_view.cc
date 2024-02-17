#include <vkt/image_view.h>

ImageView::ImageView(std::shared_ptr<Device> device,
                     ImageViewCreateInfo imageViewCreateInfo) {
  this->device = device;
  auto vk_createInfo = VkImageViewCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = VK_NULL_HANDLE,
      .flags = imageViewCreateInfo.flags,
      .image = imageViewCreateInfo.image,
      .viewType = imageViewCreateInfo.viewType,
      .format = imageViewCreateInfo.format,
      .components = imageViewCreateInfo.components,
      .subresourceRange = imageViewCreateInfo.subresourceRange};

  VkImageView imageView;
  VK_CHECK(
      device->vkCreateImageView(*device, &vk_createInfo, nullptr, &imageView));

  this->imageView = Handle<VkImageView, Device>(
      imageView,
      [](VkImageView imageView, Device &device) -> void {
        device.vkDestroyImageView(device, imageView, nullptr);
      },
      device);
}

ImageView::operator VkImageView() {
  return imageView;
}