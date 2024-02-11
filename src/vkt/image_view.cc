#include <vkt/image_view.h>

ImageView::ImageView(std::shared_ptr<Device> device,
                     ImageViewCreateInfo imageViewCreateInfo) {
  this->device = device;
  auto vk_imageViewCreateInfo = VkImageViewCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = VK_NULL_HANDLE,
      .flags = imageViewCreateInfo.flags,
      .image = imageViewCreateInfo.image,
      .viewType = imageViewCreateInfo.viewType,
      .format = imageViewCreateInfo.format,
      .components = imageViewCreateInfo.components,
      .subresourceRange = imageViewCreateInfo.subresourceRange};

  VK_CHECK(device->vkCreateImageView(*device, &vk_imageViewCreateInfo,
                                     VK_NULL_HANDLE, &imageView));
}

ImageView::ImageView(ImageView &&other) {
  device = other.device;
  imageView = other.imageView;
  other.imageView = VK_NULL_HANDLE;
}

ImageView &ImageView::operator=(ImageView &&other) {
  destroy();
  device = other.device;
  imageView = other.imageView;
  other.imageView = VK_NULL_HANDLE;
  return *this;
}

ImageView::~ImageView() {
  destroy();
}

void ImageView::destroy() {
  if (imageView != VK_NULL_HANDLE)
    device->vkDestroyImageView(*device, imageView, VK_NULL_HANDLE);
  imageView = VK_NULL_HANDLE;
}

ImageView::operator VkImageView() {
  return imageView;
}