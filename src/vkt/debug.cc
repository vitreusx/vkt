#include <vkt/debug.h>

DebugMessenger::DebugMessenger(
    std::shared_ptr<Instance> instance,
    DebugMessengerCreateInfo debugMessengerCreateInfo) {
  this->instance = instance;
  this->debugMessengerCreateInfo = std::move(debugMessengerCreateInfo);

  VkDebugUtilsMessengerCreateInfoEXT vk_debugMessengerCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = VK_NULL_HANDLE,
      .flags = {},
      .messageSeverity = this->debugMessengerCreateInfo.severity,
      .messageType = this->debugMessengerCreateInfo.type,
      .pfnUserCallback = _pfnUserCallback,
      .pUserData = this};

  instance->vkCreateDebugUtilsMessengerEXT(
      *instance, &vk_debugMessengerCreateInfo, VK_NULL_HANDLE, &messenger);
}

DebugMessenger::~DebugMessenger() {
  if (messenger)
    instance->vkDestroyDebugUtilsMessengerEXT(*instance, messenger,
                                              VK_NULL_HANDLE);
  messenger = VK_NULL_HANDLE;
}

VkBool32 DebugMessenger::_pfnUserCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
  DebugMessenger *self = reinterpret_cast<DebugMessenger *>(pUserData);
  self->debugMessengerCreateInfo.onLog(messageSeverity, messageTypes,
                                       pCallbackData);
  return VK_TRUE;
}
