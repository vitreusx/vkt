#pragma once
#include <vulkan/vulkan.h>

class Loader {
public:
  Loader(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr);

  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

#define MEMBER(name) PFN_##name name = {}
  MEMBER(vkCreateInstance);
  MEMBER(vkDestroyInstance);
  MEMBER(vkGetPhysicalDeviceProperties);
  MEMBER(vkGetPhysicalDeviceFeatures);
  MEMBER(vkGetPhysicalDeviceQueueFamilyProperties);
  MEMBER(vkCreateDevice);
  MEMBER(vkDestroyDevice);
  MEMBER(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
  MEMBER(vkGetPhysicalDeviceSurfaceFormatsKHR);
  MEMBER(vkGetPhysicalDeviceSurfacePresentModesKHR);
  MEMBER(vkGetPhysicalDeviceMemoryProperties);
  MEMBER(vkGetPhysicalDeviceSurfaceSupportKHR);
  MEMBER(vkGetPhysicalDeviceFormatProperties);
#undef MEMBER
};