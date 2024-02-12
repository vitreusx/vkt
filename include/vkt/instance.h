#pragma once
#include <memory>
#include <vector>
#include <vkt/loader.h>
#include <vkt/utils.h>
#include <vkt/phys_dev.h>
#include <optional>

struct ApiVersion {
  uint32_t variant = 0, major = 0, minor = 0, patch = 0;
  uint32_t encode() const;
};

struct ApplicationInfo {
  ApiVersion apiVersion, appVersion, engineVersion;
  std::string appName, engineName;
};

struct InstanceCreateInfo {
  VkInstanceCreateFlags flags;
  std::vector<std::string> enabledExtensions;
  std::vector<std::string> enabledLayers;
};

struct DebugMessengerCreateInfo {
  VkDebugUtilsMessageSeverityFlagsEXT severity;
  VkDebugUtilsMessageTypeFlagsEXT type;

  typedef void (*OnLog)(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                        VkDebugUtilsMessageTypeFlagsEXT type,
                        const VkDebugUtilsMessengerCallbackDataEXT *data);
  Callback<OnLog> onLog;
};

class Instance {
public:
  Instance() = default;

  Instance(
      std::shared_ptr<Loader> loader, ApplicationInfo const &appInfo,
      InstanceCreateInfo const &instanceCreateInfo,
      std::optional<DebugMessengerCreateInfo> const &debugMessengerCreateInfo);

  Instance(Instance const &) = delete;
  Instance &operator=(Instance const &) = delete;

  Instance(Instance &&) = delete;
  Instance &operator=(Instance &&) = delete;

  ~Instance();

  operator VkInstance();

  std::vector<PhysicalDevice> listPhysicalDevices();

#define INSTANCE_DEFS(MACRO)                                                   \
  MACRO(vkEnumeratePhysicalDevices);                                           \
  MACRO(vkCreateDebugUtilsMessengerEXT);                                       \
  MACRO(vkDestroyDebugUtilsMessengerEXT);                                      \
  MACRO(vkDestroySurfaceKHR)

#define MEMBER(name) PFN_##name name
  INSTANCE_DEFS(MEMBER);
#undef MEMBER

  std::shared_ptr<Loader> loader;

private:
  void loadFunctions();

  static VkBool32 _debugger_pfnUserCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageTypes,
      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
      void *pUserData);

  DebugMessengerCreateInfo debugMessengerCreateInfo;
  VkInstance instance = VK_NULL_HANDLE;
};