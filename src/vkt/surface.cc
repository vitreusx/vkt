#include <vkt/surface.h>

Surface::Surface(std::shared_ptr<Instance> instance, Window &window) {
  this->instance = instance;
  loader = instance->loader;
  glfwCreateWindowSurface(*instance, window, VK_NULL_HANDLE, &surfaceKHR);
}

Surface::Surface(Surface &&other) {
  *this = std::move(other);
}

Surface &Surface::operator=(Surface &&other) {
  destroy();
  this->instance = std::move(other.instance);
  this->loader = std::move(other.loader);
  surfaceKHR = other.surfaceKHR;
  other.surfaceKHR = VK_NULL_HANDLE;
  return *this;
}

Surface::~Surface() {
  destroy();
}

void Surface::destroy() {
  if (surfaceKHR != VK_NULL_HANDLE)
    instance->vkDestroySurfaceKHR(*instance, surfaceKHR, VK_NULL_HANDLE);
  surfaceKHR = VK_NULL_HANDLE;
}

Surface::operator VkSurfaceKHR() {
  return surfaceKHR;
}

SwapchainDetails Surface::getSwapchainDetails(VkPhysicalDevice device) {
  SwapchainDetails details;

  VK_CHECK(loader->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      device, surfaceKHR, &details.capabilities));

  uint32_t formatCount;
  VK_CHECK(loader->vkGetPhysicalDeviceSurfaceFormatsKHR(
      device, surfaceKHR, &formatCount, VK_NULL_HANDLE));

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    VK_CHECK(loader->vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, surfaceKHR, &formatCount, details.formats.data()));
  }

  uint32_t presentModeCount;
  VK_CHECK(loader->vkGetPhysicalDeviceSurfacePresentModesKHR(
      device, surfaceKHR, &presentModeCount, VK_NULL_HANDLE));

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    VK_CHECK(loader->vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surfaceKHR, &presentModeCount, details.presentModes.data()));
  }

  return details;
}