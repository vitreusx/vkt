#include <vkt/swapchain.h>

Swapchain::Swapchain(std::shared_ptr<Device> device,
                     SwapchainCreateInfo const &swapchainCreateInfo) {
  this->device = device;
  auto const &info = swapchainCreateInfo;
  auto vk_swapchainCreateInfo = VkSwapchainCreateInfoKHR{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = VK_NULL_HANDLE,
      .flags = info.flags,
      .surface = info.surface,
      .minImageCount = info.minImageCount,
      .imageFormat = info.imageFormat,
      .imageColorSpace = info.imageColorSpace,
      .imageExtent = info.imageExtent,
      .imageArrayLayers = info.imageArrayLayers,
      .imageUsage = info.imageUsage,
      .imageSharingMode = info.imageSharingMode,
      .queueFamilyIndexCount = (uint32_t)info.queueFamilyIndices.size(),
      .pQueueFamilyIndices = info.queueFamilyIndices.data(),
      .preTransform = info.preTransform,
      .compositeAlpha = info.compositeAlpha,
      .presentMode = info.presentMode,
      .clipped = info.clipped,
      .oldSwapchain = info.oldSwapchain};

  VK_CHECK(device->vkCreateSwapchainKHR(*device, &vk_swapchainCreateInfo,
                                        VK_NULL_HANDLE, &swapchain));
}

Swapchain::Swapchain(Swapchain &&other) {
  *this = std::move(other);
}

Swapchain &Swapchain::operator=(Swapchain &&other) {
  destroy();
  device = other.device;
  swapchain = other.swapchain;
  other.swapchain = VK_NULL_HANDLE;
  return *this;
}

Swapchain::~Swapchain() {
  destroy();
}

void Swapchain::destroy() {
  if (swapchain != VK_NULL_HANDLE)
    device->vkDestroySwapchainKHR(*device, swapchain, VK_NULL_HANDLE);
  swapchain = VK_NULL_HANDLE;
}

std::vector<VkImage> Swapchain::getImages() {
  uint32_t imageCount = 0;
  VK_CHECK(device->vkGetSwapchainImagesKHR(*device, swapchain, &imageCount,
                                           VK_NULL_HANDLE));

  std::vector<VkImage> swapchainImages(imageCount);
  VK_CHECK(device->vkGetSwapchainImagesKHR(*device, swapchain, &imageCount,
                                           swapchainImages.data()));

  return swapchainImages;
}

std::pair<uint32_t, VkResult> Swapchain::acquireNextImage(VkSemaphore semaphore,
                                                          VkFence fence) {
  uint32_t imageIndex;
  VkResult result = device->vkAcquireNextImageKHR(
      *device, swapchain, UINT64_MAX, semaphore, fence, &imageIndex);
  return std::make_pair(imageIndex, result);
}

Swapchain::operator VkSwapchainKHR() {
  return swapchain;
}