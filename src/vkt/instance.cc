#include <vkt/instance.h>
#include <algorithm>

uint32_t ApiVersion::encode() const {
  return VK_MAKE_API_VERSION(variant, major, minor, patch);
}

Instance::Instance(
    std::shared_ptr<Loader> loader, ApplicationInfo const &appInfo,
    InstanceCreateInfo const &instanceCreateInfo,
    std::optional<DebugMessengerCreateInfo> const &debugMessengerCreateInfo) {
  this->loader = std::move(loader);

  auto enabledExtensions = instanceCreateInfo.enabledExtensions;
  void *pNext = VK_NULL_HANDLE;
  VkDebugUtilsMessengerCreateInfoEXT vk_debugMessengerCreateInfo;

  if (debugMessengerCreateInfo.has_value()) {
    this->debugMessengerCreateInfo = std::move(*debugMessengerCreateInfo);

    vk_debugMessengerCreateInfo = VkDebugUtilsMessengerCreateInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = VK_NULL_HANDLE,
        .flags = {},
        .messageSeverity = this->debugMessengerCreateInfo.severity,
        .messageType = this->debugMessengerCreateInfo.type,
        .pfnUserCallback = _debugger_pfnUserCallback,
        .pUserData = this};

    pNext = &vk_debugMessengerCreateInfo;

    if (std::find(enabledExtensions.begin(), enabledExtensions.end(),
                  VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == enabledExtensions.end())
      enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  auto extNames = vkMapNames(enabledExtensions);
  auto layerNames = vkMapNames(instanceCreateInfo.enabledLayers);

  VkApplicationInfo vk_appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                               .pNext = VK_NULL_HANDLE,
                               .pApplicationName = appInfo.appName.c_str(),
                               .applicationVersion =
                                   appInfo.appVersion.encode(),
                               .pEngineName = appInfo.engineName.c_str(),
                               .engineVersion = appInfo.engineVersion.encode(),
                               .apiVersion = appInfo.apiVersion.encode()};

  VkInstanceCreateInfo vk_instanceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = VK_NULL_HANDLE,
      .flags = instanceCreateInfo.flags,
      .pApplicationInfo = &vk_appInfo,
      .enabledLayerCount = (uint32_t)layerNames.size(),
      .ppEnabledLayerNames = layerNames.data(),
      .enabledExtensionCount = (uint32_t)extNames.size(),
      .ppEnabledExtensionNames = extNames.data()};

  VK_CHECK(this->loader->vkCreateInstance(&vk_instanceCreateInfo,
                                          VK_NULL_HANDLE, &this->instance));

  loadFunctions();
}

Instance::~Instance() {
  if (instance != VK_NULL_HANDLE)
    loader->vkDestroyInstance(instance, VK_NULL_HANDLE);
  instance = VK_NULL_HANDLE;
}

Instance::operator VkInstance() {
  return instance;
}

std::vector<PhysicalDevice> Instance::listPhysicalDevices() {
  uint32_t physicalDeviceCount;
  VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount,
                                      VK_NULL_HANDLE));

  std::vector<VkPhysicalDevice> vk_physicalDevices(physicalDeviceCount);
  VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount,
                                      vk_physicalDevices.data()));

  std::vector<PhysicalDevice> physicalDevices;
  for (auto const &vk_physicalDevice : vk_physicalDevices)
    physicalDevices.emplace_back(*loader, vk_physicalDevice);

  return physicalDevices;
}

void Instance::loadFunctions() {
  auto vkGetInstanceProcAddr = loader->vkGetInstanceProcAddr;
#define LOAD(name)                                                             \
  this->name =                                                                 \
      reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name))
  INSTANCE_DEFS(LOAD);
#undef LOAD
}

VkBool32 Instance::_debugger_pfnUserCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
  Instance *self = reinterpret_cast<Instance *>(pUserData);
  self->debugMessengerCreateInfo.onLog(messageSeverity, messageTypes,
                                       pCallbackData);
  return VK_TRUE;
}