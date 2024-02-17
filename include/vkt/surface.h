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
  Surface() = default;
  Surface(std::shared_ptr<Instance> instance, Window &window);

  operator VkSurfaceKHR();

  SwapchainDetails getSwapchainDetails(VkPhysicalDevice device);

private:
  std::shared_ptr<Instance> instance = {};
  std::shared_ptr<Loader> loader = {};
  Handle<VkSurfaceKHR, Instance> surfaceKHR;
};