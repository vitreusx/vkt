#include <vkt/loader.h>

Loader::Loader(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr) {
  this->vkGetInstanceProcAddr = vkGetInstanceProcAddr;

#define LOAD(name)                                                             \
  this->name = reinterpret_cast<PFN_##name>(                                   \
      vkGetInstanceProcAddr(VK_NULL_HANDLE, #name))

  LOAD(vkCreateInstance);
  LOAD(vkDestroyInstance);
  LOAD(vkGetPhysicalDeviceProperties);
  LOAD(vkGetPhysicalDeviceFeatures);
  LOAD(vkGetPhysicalDeviceQueueFamilyProperties);
  LOAD(vkCreateDevice);
  LOAD(vkDestroyDevice);
  LOAD(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
  LOAD(vkGetPhysicalDeviceSurfaceFormatsKHR);
  LOAD(vkGetPhysicalDeviceSurfacePresentModesKHR);
  LOAD(vkGetPhysicalDeviceMemoryProperties);
  LOAD(vkGetPhysicalDeviceSurfaceSupportKHR);
  LOAD(vkGetPhysicalDeviceFormatProperties);
#undef LOAD
}
