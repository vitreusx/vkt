#pragma once
#include <vkt/instance.h>
#include <vkt/glfw.h>

struct SwapchainDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class Surface {
public:
  Surface(std::shared_ptr<Instance> instance, Window &window);
  Surface(Surface const &) = delete;
  Surface &operator=(Surface const &) = delete;

  Surface(Surface &&other);
  Surface &operator=(Surface &&other);

  ~Surface();

  operator VkSurfaceKHR();

  SwapchainDetails getSwapchainDetails(VkPhysicalDevice device);

private:
  std::shared_ptr<Instance> instance = {};
  std::shared_ptr<Loader> loader = {};
  VkSurfaceKHR surfaceKHR = VK_NULL_HANDLE;

  void destroy();
};