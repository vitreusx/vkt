#include <vkt/surface.h>

Surface::Surface(std::shared_ptr<Instance> instance, Window &window) {
  this->instance = instance;
  loader = instance->loader;

  VkSurfaceKHR surfaceKHR;
  glfwCreateWindowSurface(*instance, window, VK_NULL_HANDLE, &surfaceKHR);

  this->surfaceKHR = Handle<VkSurfaceKHR, Instance>(
      surfaceKHR,
      [](VkSurfaceKHR surfaceKHR, Instance &instance) -> void {
        instance.vkDestroySurfaceKHR(instance, surfaceKHR, nullptr);
      },
      instance);
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