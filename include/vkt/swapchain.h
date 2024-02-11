#pragma once
#include <vkt/device.h>

struct SwapchainCreateInfo {
  VkSwapchainCreateFlagsKHR flags;
  VkSurfaceKHR surface;
  uint32_t minImageCount;
  VkFormat imageFormat;
  VkColorSpaceKHR imageColorSpace;
  VkExtent2D imageExtent;
  uint32_t imageArrayLayers;
  VkImageUsageFlags imageUsage;
  VkSharingMode imageSharingMode;
  std::vector<uint32_t> queueFamilyIndices;
  VkSurfaceTransformFlagBitsKHR preTransform;
  VkCompositeAlphaFlagBitsKHR compositeAlpha;
  VkPresentModeKHR presentMode;
  VkBool32 clipped;
  VkSwapchainKHR oldSwapchain;
};

class Swapchain {
public:
  Swapchain() = default;

  Swapchain(std::shared_ptr<Device> device,
            SwapchainCreateInfo const &swapchainCreateInfo);

  Swapchain(Swapchain const &) = delete;
  Swapchain &operator=(Swapchain const &) = delete;

  Swapchain(Swapchain &&other);
  Swapchain &operator=(Swapchain &&other);

  ~Swapchain();

  std::vector<VkImage> getImages();

  std::pair<uint32_t, VkResult> acquireNextImage(VkSemaphore semaphore,
                                                 VkFence fence);

  operator VkSwapchainKHR();

private:
  std::shared_ptr<Device> device = {};
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;

  void destroy();
};