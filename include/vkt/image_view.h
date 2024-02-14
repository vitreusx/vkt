#pragma once
#include <vkt/device.h>

struct ImageViewCreateInfo {
  VkImageViewCreateFlags flags;
  VkImage image;
  VkImageViewType viewType;
  VkFormat format;
  VkComponentMapping components;
  VkImageSubresourceRange subresourceRange;
};

class ImageView {
public:
  ImageView() = default;

  ImageView(std::shared_ptr<Device> device,
            ImageViewCreateInfo imageViewCreateInfo);

  ImageView(ImageView const &) = delete;
  ImageView &operator=(ImageView const &) = delete;

  ImageView(ImageView &&other);
  ImageView &operator=(ImageView &&other);

  ~ImageView();
  void destroy();

  operator VkImageView();

private:
  std::shared_ptr<Device> device;
  VkImageView imageView = VK_NULL_HANDLE;
};
