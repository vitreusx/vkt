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

  operator VkImageView();

private:
  std::shared_ptr<Device> device;
  Handle<VkImageView, Device> imageView;
};
