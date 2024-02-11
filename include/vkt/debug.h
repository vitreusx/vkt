#pragma once
#include <vkt/instance.h>

class DebugMessenger {
public:
  DebugMessenger(std::shared_ptr<Instance> instance,
                 DebugMessengerCreateInfo debugMessengerCreateInfo);

  DebugMessenger(DebugMessenger const &) = delete;
  DebugMessenger &operator=(DebugMessenger const &) = delete;

  DebugMessenger(DebugMessenger &&) = delete;
  DebugMessenger &operator=(DebugMessenger &&) = delete;

  ~DebugMessenger();

private:
  std::shared_ptr<Instance> instance;
  VkDebugUtilsMessengerEXT messenger;
  DebugMessengerCreateInfo debugMessengerCreateInfo;

  static VkBool32
  _pfnUserCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                   VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                   const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                   void *pUserData);
};