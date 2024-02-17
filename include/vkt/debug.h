#pragma once
#include <vkt/instance.h>

class DebugMessenger {
public:
  DebugMessenger(std::shared_ptr<Instance> instance,
                 DebugMessengerCreateInfo debugMessengerCreateInfo);

private:
  std::shared_ptr<Instance> instance;
  Handle<VkDebugUtilsMessengerEXT, Instance> messenger;
  DebugMessengerCreateInfo debugMessengerCreateInfo;

  static VkBool32
  _pfnUserCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                   VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                   const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                   void *pUserData);
};