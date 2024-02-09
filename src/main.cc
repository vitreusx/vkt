#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <algorithm>
#include <set>
#include <fstream>
#include <chrono>

template <typename _Res, typename... _ArgTypes>
class ICallback {
public:
  virtual _Res impl(_ArgTypes... args) = 0;
};

template <typename Func, typename _Res, typename... _ArgTypes>
class LambdaCb : public ICallback<_Res, _ArgTypes...> {
public:
  LambdaCb(Func func) : func{std::move(func)} {}

  _Res impl(_ArgTypes... args) {
    if constexpr (std::is_void_v<_Res>)
      func(std::forward<_ArgTypes>(args)...);
    else
      return func(std::forward<_ArgTypes>(args)...);
  }

private:
  Func func;
};

template <typename Sig>
class Callback;

template <typename _Res, typename... _ArgTypes>
class Callback<_Res (*)(_ArgTypes...)> {
public:
  Callback() = default;

  template <typename Func>
  Callback(Func func)
      : func{std::make_shared<LambdaCb<Func, _Res, _ArgTypes...>>(
            std::move(func))} {}

  template <typename Func>
  Callback &operator=(Func func) {
    this->func =
        std::make_shared<LambdaCb<Func, _Res, _ArgTypes...>>(std::move(func));
    return *this;
  }

  _Res impl(_ArgTypes... args) {
    if (func) {
      if constexpr (std::is_void_v<_Res>)
        func->impl(std::forward<_ArgTypes>(args)...);
      else
        return func->impl(std::forward<_ArgTypes>(args)...);
    } else {
      if constexpr (!std::is_void_v<_Res>)
        return _Res();
    }
  }

  _Res operator()(_ArgTypes... args) {
    if constexpr (std::is_void_v<_Res>)
      impl(std::forward<_ArgTypes>(args)...);
    else
      return impl(std::forward<_ArgTypes>(args)...);
  }

private:
  std::shared_ptr<ICallback<_Res, _ArgTypes...>> func;
};

class GLFWLib {
public:
  static std::shared_ptr<GLFWLib> getInstance(Callback<GLFWerrorfun> cb) {
    static std::shared_ptr<GLFWLib> instance;
    if (!instance) {
      struct _GLFWLib : public GLFWLib {};
      instance = std::make_shared<_GLFWLib>();
      onError = std::move(cb);
    }
    return instance;
  }

  GLFWLib(GLFWLib const &) = delete;

private:
  GLFWLib() {
    glfwSetErrorCallback(_onError);
    glfwInit();
  }

  ~GLFWLib() {
    glfwTerminate();
  }

  static Callback<GLFWerrorfun> onError;

  static void _onError(int errorCode, char const *description) {
    onError(errorCode, description);
  }
};

// For some reason this also has to be declared outside.
Callback<GLFWerrorfun> GLFWLib::onError;

class Window {
public:
  Window(std::shared_ptr<GLFWLib> glfw, int width, int height,
         char const *title) {
    this->glfw = std::move(glfw);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    this->handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
    resetCallbacks();
  }

  Window(Window const &) = delete;
  Window(Window &&) = delete;

  ~Window() {
    if (handle)
      glfwDestroyWindow(handle);
  }

  operator GLFWwindow *() {
    return handle;
  }

  operator GLFWwindow const *() const {
    return handle;
  }

  typedef void (*OnKey)(int key, int scanCode, int action, int mods);
  Callback<OnKey> onKey;

private:
  std::shared_ptr<GLFWLib> glfw;
  GLFWwindow *handle = nullptr;

  void resetCallbacks() {
    glfwSetWindowUserPointer(this->handle, this);
    glfwSetKeyCallback(this->handle, _onKey);
  }

  static void _onKey(GLFWwindow *window, int key, int scanCode, int action,
                     int mods) {
    Window *self = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
    self->onKey(key, scanCode, action, mods);
  }
};

class Loader {
public:
  Loader(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr) {
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
#undef LOAD
  }

public:
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
#undef MEMBER
};

struct ApiVersion {
  uint32_t variant = 0, major = 0, minor = 0, patch = 0;

  uint32_t encode() const {
    return VK_MAKE_API_VERSION(variant, major, minor, patch);
  }
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

template <typename T, typename F>
auto mapV(std::vector<T> const &v, F const &f) {
  using R = decltype(std::declval<F &>()(std::declval<T const &>()));
  std::vector<R> rs;
  for (auto const &x : v)
    rs.emplace_back(f(x));
  return rs;
}

auto vkMapNames(std::vector<std::string> const &names) {
  return mapV(names, [](std::string const &s) -> auto {
    return s.c_str();
  });
}

#define VK_CHECK(expr)                                                         \
  ([&]() -> void {                                                             \
    VkResult __result = (expr);                                                \
    if (__result != VK_SUCCESS)                                                \
      throw std::runtime_error("Vk error: " #expr);                            \
  })()

struct DebugMessengerCreateInfo {
  VkDebugUtilsMessageSeverityFlagsEXT severity;
  VkDebugUtilsMessageTypeFlagsEXT type;

  typedef void (*OnLog)(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                        VkDebugUtilsMessageTypeFlagsEXT type,
                        const VkDebugUtilsMessengerCallbackDataEXT *data);
  Callback<OnLog> onLog;
};

class PhysicalDevice {
public:
  PhysicalDevice(Loader const &loader, VkPhysicalDevice physicalDevice) {
    this->physicalDevice = physicalDevice;
    loader.vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    loader.vkGetPhysicalDeviceFeatures(physicalDevice, &features);

    uint32_t queueFamilyCount = 0;
    loader.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                                    &queueFamilyCount, nullptr);

    queueFamilies.resize(queueFamilyCount);
    loader.vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice, &queueFamilyCount, queueFamilies.data());
  }

  operator VkPhysicalDevice() {
    return physicalDevice;
  }

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  std::vector<VkQueueFamilyProperties> queueFamilies;

private:
  VkPhysicalDevice physicalDevice = {};
};

class Instance {
public:
  Instance(
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
                    VK_EXT_DEBUG_UTILS_EXTENSION_NAME) ==
          enabledExtensions.end())
        enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    auto extNames = vkMapNames(enabledExtensions);
    auto layerNames = vkMapNames(instanceCreateInfo.enabledLayers);

    VkApplicationInfo vk_appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = VK_NULL_HANDLE,
        .pApplicationName = appInfo.appName.c_str(),
        .applicationVersion = appInfo.appVersion.encode(),
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

  ~Instance() {
    if (instance)
      loader->vkDestroyInstance(instance, VK_NULL_HANDLE);
  }

  operator VkInstance() {
    return instance;
  }

  std::vector<PhysicalDevice> listPhysicalDevices() {
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

#define MEMBER(name) PFN_##name name
  MEMBER(vkEnumeratePhysicalDevices);
  MEMBER(vkCreateDebugUtilsMessengerEXT);
  MEMBER(vkDestroyDebugUtilsMessengerEXT);
  MEMBER(vkDestroySurfaceKHR);
#undef MEMBER

  std::shared_ptr<Loader> loader;

private:
  void loadFunctions() {
    auto vkGetInstanceProcAddr = loader->vkGetInstanceProcAddr;
#define LOAD(name)                                                             \
  this->name =                                                                 \
      reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name))
    LOAD(vkEnumeratePhysicalDevices);
    LOAD(vkCreateDebugUtilsMessengerEXT);
    LOAD(vkDestroyDebugUtilsMessengerEXT);
    LOAD(vkDestroySurfaceKHR);
#undef LOAD
  }

  static VkBool32 _debugger_pfnUserCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageTypes,
      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
      void *pUserData) {
    Instance *self = reinterpret_cast<Instance *>(pUserData);
    self->debugMessengerCreateInfo.onLog(messageSeverity, messageTypes,
                                         pCallbackData);
    return VK_TRUE;
  }

  DebugMessengerCreateInfo debugMessengerCreateInfo;
  VkInstance instance = {};
};

template <typename Iter, typename Key>
auto find_max(Iter begin, Iter end, Key key) {
  Iter best = begin;
  auto bestScore = key(*best);
  for (Iter cur = begin; cur != end; ++cur) {
    auto curScore = key(*cur);
    if (bestScore < curScore) {
      best = cur;
      bestScore = curScore;
    }
  }
  return best;
}

struct DeviceQueueCreateInfo {
  VkDeviceQueueCreateFlags flags = {};
  uint32_t queueFamilyIndex = {};
  std::vector<float> queuePriorities = {};
};

struct DeviceCreateInfo {
  VkDeviceCreateFlags flags = {};
  std::vector<DeviceQueueCreateInfo> queueCreateInfos = {};
  std::vector<std::string> enabledLayers = {};
  std::vector<std::string> enabledExtensions = {};
  VkPhysicalDeviceFeatures enabledFeatures = {};
};

struct QueueSubmitInfo {
  std::vector<std::pair<VkSemaphore, VkPipelineStageFlags>>
      waitSemaphoresAndStages;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> signalSemaphores;
  VkFence fence;
};

struct QueuePresentInfo {
  std::vector<VkSemaphore> waitSemaphores;
  std::vector<std::pair<VkSwapchainKHR, uint32_t>> swapchainsAndImageIndices;
};

class Device {
public:
  Device(VkDevice &&device) : device{std::move(device)} {}

  Device(std::shared_ptr<Loader> loader, VkPhysicalDevice physicalDevice,
         DeviceCreateInfo const &deviceCreateInfo) {
    this->loader = loader;

    auto vk_queueCreateInfos = mapV(
        deviceCreateInfo.queueCreateInfos,
        [](auto const &queueCreateInfo) -> auto {
          return VkDeviceQueueCreateInfo{
              .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
              .pNext = VK_NULL_HANDLE,
              .flags = queueCreateInfo.flags,
              .queueFamilyIndex = queueCreateInfo.queueFamilyIndex,
              .queueCount = (uint32_t)queueCreateInfo.queuePriorities.size(),
              .pQueuePriorities = queueCreateInfo.queuePriorities.data()};
        });

    auto extNames = vkMapNames(deviceCreateInfo.enabledExtensions);
    auto layerNames = vkMapNames(deviceCreateInfo.enabledLayers);

    VkDeviceCreateInfo vk_deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = deviceCreateInfo.flags,
        .queueCreateInfoCount = (uint32_t)vk_queueCreateInfos.size(),
        .pQueueCreateInfos = vk_queueCreateInfos.data(),
        .enabledLayerCount = (uint32_t)layerNames.size(),
        .ppEnabledLayerNames = layerNames.data(),
        .enabledExtensionCount = (uint32_t)extNames.size(),
        .ppEnabledExtensionNames = extNames.data(),
        .pEnabledFeatures = &deviceCreateInfo.enabledFeatures};

    VK_CHECK(loader->vkCreateDevice(physicalDevice, &vk_deviceCreateInfo,
                                    VK_NULL_HANDLE, &device));

    loadFunctions();
  }

  Device(Device const &) = delete;
  Device(Device &&) = delete;

  ~Device() {
    if (device)
      loader->vkDestroyDevice(device, VK_NULL_HANDLE);
  }

  operator VkDevice() {
    return device;
  }

#define MEMBER(name) PFN_##name name
  MEMBER(vkGetDeviceQueue);
  MEMBER(vkCreateSwapchainKHR);
  MEMBER(vkDestroySwapchainKHR);
  MEMBER(vkGetSwapchainImagesKHR);
  MEMBER(vkCreateImageView);
  MEMBER(vkDestroyImageView);
  MEMBER(vkCreateShaderModule);
  MEMBER(vkDestroyShaderModule);
  MEMBER(vkCreatePipelineLayout);
  MEMBER(vkDestroyPipelineLayout);
  MEMBER(vkCreateGraphicsPipelines);
  MEMBER(vkDestroyPipeline);
  MEMBER(vkCreateRenderPass);
  MEMBER(vkDestroyRenderPass);
  MEMBER(vkCreateFramebuffer);
  MEMBER(vkDestroyFramebuffer);
  MEMBER(vkCreateCommandPool);
  MEMBER(vkDestroyCommandPool);
  MEMBER(vkAllocateCommandBuffers);
  MEMBER(vkFreeCommandBuffers);
  MEMBER(vkCreateSemaphore);
  MEMBER(vkDestroySemaphore);
  MEMBER(vkCreateFence);
  MEMBER(vkDestroyFence);
  MEMBER(vkWaitForFences);
  MEMBER(vkResetFences);
  MEMBER(vkAcquireNextImageKHR);
  MEMBER(vkQueueSubmit);
  MEMBER(vkQueuePresentKHR);
#undef MEMBER

private:
  std::shared_ptr<Loader> loader = {};
  VkDevice device = {};

  void loadFunctions() {
#define LOAD(name) this->name = (PFN_##name)vkGetDeviceProcAddr(device, #name)
    LOAD(vkGetDeviceQueue);
    LOAD(vkCreateSwapchainKHR);
    LOAD(vkDestroySwapchainKHR);
    LOAD(vkGetSwapchainImagesKHR);
    LOAD(vkCreateImageView);
    LOAD(vkDestroyImageView);
    LOAD(vkCreateShaderModule);
    LOAD(vkDestroyShaderModule);
    LOAD(vkCreatePipelineLayout);
    LOAD(vkDestroyPipelineLayout);
    LOAD(vkCreateGraphicsPipelines);
    LOAD(vkDestroyPipeline);
    LOAD(vkCreateRenderPass);
    LOAD(vkDestroyRenderPass);
    LOAD(vkCreateFramebuffer);
    LOAD(vkDestroyFramebuffer);
    LOAD(vkCreateCommandPool);
    LOAD(vkDestroyCommandPool);
    LOAD(vkAllocateCommandBuffers);
    LOAD(vkFreeCommandBuffers);
    LOAD(vkCreateSemaphore);
    LOAD(vkDestroySemaphore);
    LOAD(vkCreateFence);
    LOAD(vkDestroyFence);
    LOAD(vkWaitForFences);
    LOAD(vkResetFences);
    LOAD(vkAcquireNextImageKHR);
    LOAD(vkQueueSubmit);
    LOAD(vkQueuePresentKHR);
#undef LOAD
  }
};

class Queue {
public:
  Queue(std::shared_ptr<Device> device, uint32_t queueFamilyIndex,
        uint32_t queueIndex) {
    this->device = device;
    device->vkGetDeviceQueue(*device, queueFamilyIndex, queueIndex, &queue);
  }

  operator VkQueue() {
    return queue;
  }

  void submit(QueueSubmitInfo const &submitInfo) {
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitDstStageMasks;
    for (auto const &[semaphore, stage] : submitInfo.waitSemaphoresAndStages) {
      waitSemaphores.push_back(semaphore);
      waitDstStageMasks.push_back(stage);
    }

    auto vk_submitInfo = VkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = (uint32_t)waitSemaphores.size(),
        .pWaitSemaphores = waitSemaphores.data(),
        .pWaitDstStageMask = waitDstStageMasks.data(),
        .commandBufferCount = (uint32_t)submitInfo.commandBuffers.size(),
        .pCommandBuffers = submitInfo.commandBuffers.data(),
        .signalSemaphoreCount = (uint32_t)submitInfo.signalSemaphores.size(),
        .pSignalSemaphores = submitInfo.signalSemaphores.data()};

    VK_CHECK(device->vkQueueSubmit(queue, 1, &vk_submitInfo, submitInfo.fence));
  }

  void present(QueuePresentInfo const &presentInfo) {
    std::vector<VkSwapchainKHR> swapchains;
    std::vector<uint32_t> imageIndices;
    for (auto const &[swapchain, imageIndex] :
         presentInfo.swapchainsAndImageIndices) {
      swapchains.push_back(swapchain);
      imageIndices.push_back(imageIndex);
    }

    auto vk_presentInfo = VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = (uint32_t)presentInfo.waitSemaphores.size(),
        .pWaitSemaphores = presentInfo.waitSemaphores.data(),
        .swapchainCount = (uint32_t)swapchains.size(),
        .pSwapchains = swapchains.data(),
        .pImageIndices = imageIndices.data(),
        .pResults = nullptr};

    device->vkQueuePresentKHR(queue, &vk_presentInfo);
  }

private:
  std::shared_ptr<Device> device = {};
  VkQueue queue = VK_NULL_HANDLE;
};

class DebugMessenger {
public:
  DebugMessenger(std::shared_ptr<Instance> instance,
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

  DebugMessenger(DebugMessenger const &) = delete;
  DebugMessenger(DebugMessenger &&) = delete;

  ~DebugMessenger() {
    if (messenger)
      instance->vkDestroyDebugUtilsMessengerEXT(*instance, messenger,
                                                VK_NULL_HANDLE);
  }

private:
  std::shared_ptr<Instance> instance;
  VkDebugUtilsMessengerEXT messenger;
  DebugMessengerCreateInfo debugMessengerCreateInfo;

  static VkBool32
  _pfnUserCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                   VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                   const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                   void *pUserData) {
    DebugMessenger *self = reinterpret_cast<DebugMessenger *>(pUserData);
    self->debugMessengerCreateInfo.onLog(messageSeverity, messageTypes,
                                         pCallbackData);
    return VK_TRUE;
  }
};

struct SwapchainDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class Surface {
public:
  Surface(std::shared_ptr<Instance> instance, Window &window) {
    this->instance = instance;
    loader = instance->loader;
    glfwCreateWindowSurface(*instance, window, VK_NULL_HANDLE, &surfaceKHR);
  }

  Surface(Surface const &) = delete;
  Surface(Surface &&) = delete;

  ~Surface() {
    instance->vkDestroySurfaceKHR(*instance, surfaceKHR, VK_NULL_HANDLE);
  }

  operator VkSurfaceKHR() {
    return surfaceKHR;
  }

  SwapchainDetails getSwapchainDetails(VkPhysicalDevice device) {
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

private:
  std::shared_ptr<Instance> instance = {};
  std::shared_ptr<Loader> loader = {};
  VkSurfaceKHR surfaceKHR = {};
};

struct SwapchainCreateInfo {
  VkSwapchainCreateFlagsKHR flags;
  VkSurfaceKHR surface;
  uint32_t minImageCount;
  VkFormat imageFormat;
  VkColorSpaceKHR imageColorSpace;
  VkExtent2D imageExtent;
  uint32_t imageArrayLayers;
  VkImageUsageFlags imageUsage;
  VkSharingMode imageSharingMode;
  std::vector<uint32_t> queueFamilyIndices;
  VkSurfaceTransformFlagBitsKHR preTransform;
  VkCompositeAlphaFlagBitsKHR compositeAlpha;
  VkPresentModeKHR presentMode;
  VkBool32 clipped;
  VkSwapchainKHR oldSwapchain;
};

class Swapchain {
public:
  Swapchain(std::shared_ptr<Device> device,
            SwapchainCreateInfo const &swapchainCreateInfo) {
    this->device = device;
    auto const &info = swapchainCreateInfo;
    auto vk_swapchainCreateInfo = VkSwapchainCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = VK_NULL_HANDLE,
        .flags = info.flags,
        .surface = info.surface,
        .minImageCount = info.minImageCount,
        .imageFormat = info.imageFormat,
        .imageColorSpace = info.imageColorSpace,
        .imageExtent = info.imageExtent,
        .imageArrayLayers = info.imageArrayLayers,
        .imageUsage = info.imageUsage,
        .imageSharingMode = info.imageSharingMode,
        .queueFamilyIndexCount = (uint32_t)info.queueFamilyIndices.size(),
        .pQueueFamilyIndices = info.queueFamilyIndices.data(),
        .preTransform = info.preTransform,
        .compositeAlpha = info.compositeAlpha,
        .presentMode = info.presentMode,
        .clipped = info.clipped,
        .oldSwapchain = info.oldSwapchain};

    VK_CHECK(device->vkCreateSwapchainKHR(*device, &vk_swapchainCreateInfo,
                                          VK_NULL_HANDLE, &swapchain));
  }

  Swapchain(Swapchain const &) = delete;
  Swapchain(Swapchain &&) = delete;

  ~Swapchain() {
    device->vkDestroySwapchainKHR(*device, swapchain, VK_NULL_HANDLE);
  }

  std::vector<VkImage> getImages() {
    uint32_t imageCount = 0;
    VK_CHECK(device->vkGetSwapchainImagesKHR(*device, swapchain, &imageCount,
                                             VK_NULL_HANDLE));

    std::vector<VkImage> swapchainImages(imageCount);
    VK_CHECK(device->vkGetSwapchainImagesKHR(*device, swapchain, &imageCount,
                                             swapchainImages.data()));

    return swapchainImages;
  }

  uint32_t acquireNextImage(VkSemaphore semaphore, VkFence fence) {
    uint32_t imageIndex;
    device->vkAcquireNextImageKHR(*device, swapchain, UINT64_MAX, semaphore,
                                  fence, &imageIndex);
    return imageIndex;
  }

  operator VkSwapchainKHR() {
    return swapchain;
  }

private:
  std::shared_ptr<Device> device = {};
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
};

struct ImageViewCreateInfo {
  VkImageViewCreateFlags flags;
  VkImage image;
  VkImageViewType viewType;
  VkFormat format;
  VkComponentMapping components;
  VkImageSubresourceRange subresourceRange;
};

class ImageView {
public:
  ImageView(std::shared_ptr<Device> device,
            ImageViewCreateInfo imageViewCreateInfo) {
    this->device = device;
    auto vk_imageViewCreateInfo = VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = imageViewCreateInfo.flags,
        .image = imageViewCreateInfo.image,
        .viewType = imageViewCreateInfo.viewType,
        .format = imageViewCreateInfo.format,
        .components = imageViewCreateInfo.components,
        .subresourceRange = imageViewCreateInfo.subresourceRange};

    VK_CHECK(device->vkCreateImageView(*device, &vk_imageViewCreateInfo,
                                       VK_NULL_HANDLE, &imageView));
  }

  ImageView(ImageView const &) = delete;
  ImageView &operator=(ImageView const &) = delete;

  ImageView(ImageView &&other) {
    device = other.device;
    imageView = other.imageView;
    other.imageView = VK_NULL_HANDLE;
  }

  ImageView &operator=(ImageView &&other) {
    this->~ImageView();
    device = other.device;
    imageView = other.imageView;
    other.imageView = VK_NULL_HANDLE;
    return *this;
  }

  ~ImageView() {
    device->vkDestroyImageView(*device, imageView, VK_NULL_HANDLE);
  }

  operator VkImageView() {
    return imageView;
  }

private:
  std::shared_ptr<Device> device;
  VkImageView imageView = VK_NULL_HANDLE;
};

struct ShaderModuleCreateInfo {
  std::string code;
};

class ShaderModule {
public:
  ShaderModule(std::shared_ptr<Device> device,
               ShaderModuleCreateInfo const &createInfo) {
    this->device = device;

    VkShaderModuleCreateInfo vk_createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .codeSize = createInfo.code.size(),
        .pCode = reinterpret_cast<const uint32_t *>(createInfo.code.c_str())};

    VK_CHECK(device->vkCreateShaderModule(*device, &vk_createInfo,
                                          VK_NULL_HANDLE, &shaderModule));
  }

  ShaderModule(ShaderModule const &) = delete;
  ShaderModule(ShaderModule &&) = delete;

  ~ShaderModule() {
    device->vkDestroyShaderModule(*device, shaderModule, VK_NULL_HANDLE);
  }

  operator VkShaderModule() {
    return shaderModule;
  }

private:
  std::shared_ptr<Device> device = {};
  VkShaderModule shaderModule = {};
};

std::string readFile(std::ifstream &is) {
  std::stringstream buffer;
  buffer << is.rdbuf();
  return buffer.str();
}

struct PipelineLayoutCreateInfo {
  std::vector<VkDescriptorSetLayout> setLayouts;
  std::vector<VkPushConstantRange> pushConstantRanges;
};

class PipelineLayout {
public:
  PipelineLayout(std::shared_ptr<Device> device,
                 PipelineLayoutCreateInfo createInfo) {
    this->device = device;
    VkPipelineLayoutCreateInfo vk_createInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = {},
        .setLayoutCount = (uint32_t)createInfo.setLayouts.size(),
        .pSetLayouts = createInfo.setLayouts.data(),
        .pushConstantRangeCount =
            (uint32_t)createInfo.pushConstantRanges.size(),
        .pPushConstantRanges = createInfo.pushConstantRanges.data()};

    VK_CHECK(device->vkCreatePipelineLayout(*device, &vk_createInfo,
                                            VK_NULL_HANDLE, &pipelineLayout));
  }

  PipelineLayout(PipelineLayout const &) = delete;
  PipelineLayout(PipelineLayout &&) = delete;

  ~PipelineLayout() {
    device->vkDestroyPipelineLayout(*device, pipelineLayout, VK_NULL_HANDLE);
  }

  operator VkPipelineLayout() {
    return pipelineLayout;
  }

private:
  std::shared_ptr<Device> device = {};
  VkPipelineLayout pipelineLayout = {};
};

struct SubpassDescription {
  VkSubpassDescriptionFlags flags;
  VkPipelineBindPoint pipelineBindPoint;
  VkAttachmentReference depthStencil;
  std::vector<VkAttachmentReference> input, color, resolve;
  std::vector<uint32_t> preserve;
};

struct RenderPassCreateInfo {
  std::vector<VkAttachmentDescription> attachments;
  std::vector<SubpassDescription> subpasses;
  std::vector<VkSubpassDependency> dependencies;
};

class RenderPass {
public:
  RenderPass(std::shared_ptr<Device> device,
             RenderPassCreateInfo const &createInfo) {
    this->device = device;

    auto vk_subpasses = mapV(
        createInfo.subpasses, [](SubpassDescription const &subpass) -> auto {
          return VkSubpassDescription{
              .flags = subpass.flags,
              .pipelineBindPoint = subpass.pipelineBindPoint,
              .inputAttachmentCount = (uint32_t)subpass.input.size(),
              .pInputAttachments = subpass.input.data(),
              .colorAttachmentCount = (uint32_t)subpass.color.size(),
              .pColorAttachments = subpass.color.data(),
              .pResolveAttachments = subpass.resolve.data(),
              .pDepthStencilAttachment = &subpass.depthStencil,
              .preserveAttachmentCount = (uint32_t)subpass.preserve.size(),
              .pPreserveAttachments = subpass.preserve.data()};
        });

    auto vk_createInfo = VkRenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = {},
        .attachmentCount = (uint32_t)createInfo.attachments.size(),
        .pAttachments = createInfo.attachments.data(),
        .subpassCount = (uint32_t)vk_subpasses.size(),
        .pSubpasses = vk_subpasses.data(),
        .dependencyCount = (uint32_t)createInfo.dependencies.size(),
        .pDependencies = createInfo.dependencies.data()};

    VK_CHECK(device->vkCreateRenderPass(*device, &vk_createInfo, VK_NULL_HANDLE,
                                        &renderPass));
  }

  RenderPass(RenderPass const &) = delete;
  RenderPass(RenderPass &&) = delete;

  ~RenderPass() {
    device->vkDestroyRenderPass(*device, renderPass, VK_NULL_HANDLE);
  }

  operator VkRenderPass() {
    return renderPass;
  }

private:
  std::shared_ptr<Device> device = {};
  VkRenderPass renderPass = {};
};

class Pipeline {
public:
  Pipeline() = default;

  Pipeline(std::shared_ptr<Device> device, VkPipeline &&pipeline)
      : device{std::move(device)}, pipeline{std::move(pipeline)} {}

  Pipeline(Pipeline const &) = delete;
  Pipeline(Pipeline &&) = delete;

  ~Pipeline() {
    device->vkDestroyPipeline(*device, pipeline, VK_NULL_HANDLE);
  }

  operator VkPipeline() {
    return pipeline;
  }

protected:
  std::shared_ptr<Device> device = {};
  VkPipeline pipeline = VK_NULL_HANDLE;
};

struct SpecializationInfo {
  std::vector<VkSpecializationMapEntry> mapEntries;
  size_t dataSize;
  const void *data;
};

struct ShaderStageCreateInfo {
  VkShaderStageFlagBits stage;
  std::shared_ptr<ShaderModule> module;
  std::string name;
  std::optional<SpecializationInfo> specializationInfo;
};

struct VertexInputStateCreateInfo {
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;
};

struct InputAssemblyStateCreateInfo {
  VkPrimitiveTopology topology;
  VkBool32 primitiveRestartEnable;
};

struct ViewportStateCreateInfo {
  std::variant<std::vector<VkRect2D>, uint32_t> scissors;
  std::variant<std::vector<VkViewport>, uint32_t> viewports;
};

struct RasterizationStateCreateInfo {
  VkBool32 depthClampEnable;
  VkBool32 rasterizerDiscardEnable;
  VkPolygonMode polygonMode;
  VkCullModeFlags cullMode;
  VkFrontFace frontFace;
  VkBool32 depthBiasEnable;
  float depthBiasConstantFactor;
  float depthBiasClamp;
  float depthBiasSlopeFactor;
  float lineWidth;
};

struct MultisampleStateCreateInfo {
  VkSampleCountFlagBits rasterizationSamples;
  VkBool32 sampleShadingEnable;
  float minSampleShading;
  std::optional<VkSampleMask> sampleMask;
  VkBool32 alphaToCoverageEnable;
  VkBool32 alphaToOneEnable;
};

struct ColorBlendStateCreateInfo {
  VkBool32 logicOpEnable;
  VkLogicOp logicOp;
  std::vector<VkPipelineColorBlendAttachmentState> attachments;
  std::array<float, 4> blendConstants;
};

struct GraphicsPipelineCreateInfo {
  std::vector<ShaderStageCreateInfo> shaderStages;
  VertexInputStateCreateInfo vertexInputState;
  InputAssemblyStateCreateInfo inputAssemblyState;
  ViewportStateCreateInfo viewportState;
  RasterizationStateCreateInfo rasterizationState;
  MultisampleStateCreateInfo multisampleState;
  ColorBlendStateCreateInfo colorBlendState;
  std::vector<VkDynamicState> dynamicStates;
  std::shared_ptr<PipelineLayout> pipelineLayout;
  std::shared_ptr<RenderPass> renderPass;
  uint32_t subpass;
};

class GraphicsPipeline : public Pipeline {
public:
  GraphicsPipeline(std::shared_ptr<Device> device,
                   GraphicsPipelineCreateInfo const &createInfo) {
    this->device = device;
    this->pipelineLayout = createInfo.pipelineLayout;
    this->renderPass = createInfo.renderPass;

    std::vector<VkSpecializationInfo> vk_specializationInfos;
    std::vector<VkPipelineShaderStageCreateInfo> vk_shaderStages;
    for (auto const &shaderStage : createInfo.shaderStages) {
      auto const &specializationInfo = shaderStage.specializationInfo;

      VkSpecializationInfo const *pSpecializationInfo = nullptr;
      if (specializationInfo.has_value()) {
        pSpecializationInfo =
            &vk_specializationInfos.emplace_back(VkSpecializationInfo{
                .mapEntryCount =
                    (uint32_t)specializationInfo->mapEntries.size(),
                .pMapEntries = specializationInfo->mapEntries.data(),
                .dataSize = specializationInfo->dataSize,
                .pData = specializationInfo->data});
      }

      vk_shaderStages.emplace_back(VkPipelineShaderStageCreateInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = VK_NULL_HANDLE,
          .flags = {},
          .stage = shaderStage.stage,
          .module = *shaderStage.module,
          .pName = shaderStage.name.c_str(),
          .pSpecializationInfo = pSpecializationInfo});
    }

    auto const &vertexInputState = createInfo.vertexInputState;
    auto vk_vertexInputState = VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = {},
        .vertexBindingDescriptionCount =
            (uint32_t)vertexInputState.bindings.size(),
        .pVertexBindingDescriptions = vertexInputState.bindings.data(),
        .vertexAttributeDescriptionCount =
            (uint32_t)vertexInputState.attributes.size(),
        .pVertexAttributeDescriptions = vertexInputState.attributes.data()};

    auto const &inputAssemblyState = createInfo.inputAssemblyState;
    auto vk_inputAssemblyState = VkPipelineInputAssemblyStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = {},
        .topology = inputAssemblyState.topology,
        .primitiveRestartEnable = inputAssemblyState.primitiveRestartEnable};

    auto const &viewportState = createInfo.viewportState;

    uint32_t viewportCount;
    const VkViewport *pViewports;
    if (std::holds_alternative<uint32_t>(viewportState.viewports)) {
      viewportCount = std::get<uint32_t>(viewportState.viewports);
      pViewports = VK_NULL_HANDLE;
    } else {
      auto const &viewports =
          std::get<std::vector<VkViewport>>(viewportState.viewports);
      viewportCount = (uint32_t)viewports.size();
      pViewports = viewports.data();
    }

    uint32_t scissorCount;
    const VkRect2D *pScissors;
    if (std::holds_alternative<uint32_t>(viewportState.scissors)) {
      scissorCount = std::get<uint32_t>(viewportState.scissors);
      pScissors = VK_NULL_HANDLE;
    } else {
      auto const &scissors =
          std::get<std::vector<VkRect2D>>(viewportState.scissors);
      scissorCount = (uint32_t)scissors.size();
      pScissors = scissors.data();
    }

    auto vk_viewportState = VkPipelineViewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = {},
        .viewportCount = viewportCount,
        .pViewports = pViewports,
        .scissorCount = scissorCount,
        .pScissors = pScissors};

    auto const &rasterizationState = createInfo.rasterizationState;
    auto vk_rasterizationState = VkPipelineRasterizationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = {},
        .depthClampEnable = rasterizationState.depthClampEnable,
        .rasterizerDiscardEnable = rasterizationState.rasterizerDiscardEnable,
        .polygonMode = rasterizationState.polygonMode,
        .cullMode = rasterizationState.cullMode,
        .frontFace = rasterizationState.frontFace,
        .depthBiasEnable = rasterizationState.depthBiasEnable,
        .depthBiasConstantFactor = rasterizationState.depthBiasConstantFactor,
        .depthBiasClamp = rasterizationState.depthBiasClamp,
        .depthBiasSlopeFactor = rasterizationState.depthBiasSlopeFactor,
        .lineWidth = rasterizationState.lineWidth};

    auto const &multisampleState = createInfo.multisampleState;

    const VkSampleMask *pSampleMask = nullptr;
    if (multisampleState.sampleMask.has_value())
      pSampleMask = &multisampleState.sampleMask.value();

    auto vk_multisampleState = VkPipelineMultisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = {},
        .rasterizationSamples = multisampleState.rasterizationSamples,
        .sampleShadingEnable = multisampleState.sampleShadingEnable,
        .minSampleShading = multisampleState.minSampleShading,
        .pSampleMask = pSampleMask,
        .alphaToCoverageEnable = multisampleState.alphaToCoverageEnable,
        .alphaToOneEnable = multisampleState.alphaToOneEnable};

    auto const &colorBlendState = createInfo.colorBlendState;
    auto vk_colorBlendState = VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = {},
        .logicOpEnable = colorBlendState.logicOpEnable,
        .logicOp = colorBlendState.logicOp,
        .attachmentCount = (uint32_t)colorBlendState.attachments.size(),
        .pAttachments = colorBlendState.attachments.data(),
        .blendConstants = {colorBlendState.blendConstants[0],
                           colorBlendState.blendConstants[1],
                           colorBlendState.blendConstants[2],
                           colorBlendState.blendConstants[3]}};

    auto vk_dynamicState = VkPipelineDynamicStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = {},
        .dynamicStateCount = (uint32_t)createInfo.dynamicStates.size(),
        .pDynamicStates = createInfo.dynamicStates.data()};

    auto vk_createInfo = VkGraphicsPipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = {},
        .stageCount = (uint32_t)vk_shaderStages.size(),
        .pStages = vk_shaderStages.data(),
        .pVertexInputState = &vk_vertexInputState,
        .pInputAssemblyState = &vk_inputAssemblyState,
        .pTessellationState = VK_NULL_HANDLE,
        .pViewportState = &vk_viewportState,
        .pRasterizationState = &vk_rasterizationState,
        .pMultisampleState = &vk_multisampleState,
        .pDepthStencilState = VK_NULL_HANDLE,
        .pColorBlendState = &vk_colorBlendState,
        .pDynamicState = &vk_dynamicState,
        .layout = *createInfo.pipelineLayout,
        .renderPass = *createInfo.renderPass,
        .subpass = createInfo.subpass,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1};

    VK_CHECK(device->vkCreateGraphicsPipelines(
        *device, VK_NULL_HANDLE, 1, &vk_createInfo, VK_NULL_HANDLE, &pipeline));
  }

  GraphicsPipeline(GraphicsPipeline const &) = delete;
  GraphicsPipeline(GraphicsPipeline &&) = delete;

private:
  std::shared_ptr<PipelineLayout> pipelineLayout = {};
  std::shared_ptr<RenderPass> renderPass = {};
};

struct FramebufferCreateInfo {
  std::vector<std::shared_ptr<ImageView>> attachments;
  std::shared_ptr<RenderPass> renderPass;
  VkExtent2D extent;
  uint32_t layers;
};

class Framebuffer {
public:
  Framebuffer(std::shared_ptr<Device> device,
              FramebufferCreateInfo const &createInfo) {
    this->device = device;
    this->renderPass = createInfo.renderPass;
    this->attachments = createInfo.attachments;

    std::vector<VkImageView> vk_attachments =
        mapV(createInfo.attachments, [](auto const &imageView) -> auto {
          return (VkImageView)*imageView;
        });

    auto vk_createInfo = VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .renderPass = *createInfo.renderPass,
        .attachmentCount = (uint32_t)createInfo.attachments.size(),
        .pAttachments = vk_attachments.data(),
        .width = createInfo.extent.width,
        .height = createInfo.extent.height,
        .layers = createInfo.layers};

    VK_CHECK(device->vkCreateFramebuffer(*device, &vk_createInfo, nullptr,
                                         &framebuffer));
  }

  Framebuffer(Framebuffer const &) = delete;
  Framebuffer(Framebuffer &&) = delete;

  ~Framebuffer() {
    device->vkDestroyFramebuffer(*device, framebuffer, nullptr);
  }

  operator VkFramebuffer() {
    return framebuffer;
  }

private:
  std::shared_ptr<Device> device = {};
  std::shared_ptr<RenderPass> renderPass = {};
  VkFramebuffer framebuffer = VK_NULL_HANDLE;
  std::vector<std::shared_ptr<ImageView>> attachments = {};
};

struct CommandPoolCreateInfo {
  VkCommandPoolCreateFlags flags;
  uint32_t queueFamilyIndex;
};

class CommandPool {
public:
  CommandPool(std::shared_ptr<Device> device,
              CommandPoolCreateInfo const &createInfo) {
    this->device = device;

    auto vk_createInfo = VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = createInfo.flags,
        .queueFamilyIndex = createInfo.queueFamilyIndex};

    VK_CHECK(device->vkCreateCommandPool(*device, &vk_createInfo, nullptr,
                                         &commandPool));
  }

  CommandPool(CommandPool const &) = delete;
  CommandPool(CommandPool &&) = delete;

  ~CommandPool() {
    if (commandPool != VK_NULL_HANDLE)
      device->vkDestroyCommandPool(*device, commandPool, nullptr);
  }

  operator VkCommandPool() {
    return commandPool;
  }

private:
  std::shared_ptr<Device> device = {};
  VkCommandPool commandPool = VK_NULL_HANDLE;
};

struct CommandBufferAllocateInfo {
  std::shared_ptr<CommandPool> commandPool;
  VkCommandBufferLevel level;
};

class CommandBuffer {
public:
  CommandBuffer(std::shared_ptr<Device> device,
                CommandBufferAllocateInfo allocInfo) {
    this->device = device;
    this->commandPool = allocInfo.commandPool;

    VkCommandBufferAllocateInfo vk_allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = *allocInfo.commandPool,
        .level = allocInfo.level,
        .commandBufferCount = 1,
    };

    VK_CHECK(device->vkAllocateCommandBuffers(*device, &vk_allocInfo,
                                              &commandBuffer));

    loadFunctions();
  }

  CommandBuffer(CommandBuffer const &) = delete;
  CommandBuffer(CommandBuffer &&) = delete;

  ~CommandBuffer() {
    if (commandBuffer != VK_NULL_HANDLE)
      device->vkFreeCommandBuffers(*device, *commandPool, 1, &commandBuffer);
  }

  operator VkCommandBuffer() {
    return commandBuffer;
  }

  void reset(VkCommandBufferResetFlags flags = {}) {
    VK_CHECK(vkResetCommandBuffer(commandBuffer, flags));
  }

#define MEMBER(name) PFN_##name name
  MEMBER(vkBeginCommandBuffer);
  MEMBER(vkEndCommandBuffer);
  MEMBER(vkCmdBeginRenderPass);
  MEMBER(vkCmdEndRenderPass);
  MEMBER(vkCmdClearColorImage);
  MEMBER(vkCmdBindPipeline);
  MEMBER(vkCmdSetViewport);
  MEMBER(vkCmdSetScissor);
  MEMBER(vkCmdDraw);
  MEMBER(vkResetCommandBuffer);
  MEMBER(vkCmdPipelineBarrier);
#undef MEMBER

private:
  std::shared_ptr<Device> device;
  std::shared_ptr<CommandPool> commandPool;
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

  void loadFunctions() {
#define LOAD(name) this->name = (PFN_##name)vkGetDeviceProcAddr(*device, #name)
    LOAD(vkBeginCommandBuffer);
    LOAD(vkEndCommandBuffer);
    LOAD(vkCmdBeginRenderPass);
    LOAD(vkCmdEndRenderPass);
    LOAD(vkCmdClearColorImage);
    LOAD(vkCmdBindPipeline);
    LOAD(vkCmdSetViewport);
    LOAD(vkCmdSetScissor);
    LOAD(vkCmdDraw);
    LOAD(vkResetCommandBuffer);
    LOAD(vkCmdPipelineBarrier);
#undef LOAD
  }
};

struct CommandBufferBeginInfo {
  VkCommandBufferUsageFlags flags;
};

class CommandBufferRecording {
public:
  CommandBufferRecording(std::shared_ptr<CommandBuffer> commandBuffer,
                         CommandBufferBeginInfo const &beginInfo) {
    this->commandBuffer = commandBuffer;

    auto vk_beginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = beginInfo.flags,
        .pInheritanceInfo = nullptr};

    VK_CHECK(
        commandBuffer->vkBeginCommandBuffer(*commandBuffer, &vk_beginInfo));
  }

  CommandBufferRecording(CommandBufferRecording const &) = delete;
  CommandBufferRecording(CommandBufferRecording &&) = delete;

  ~CommandBufferRecording() {
    VK_CHECK(commandBuffer->vkEndCommandBuffer(*commandBuffer));
  }

  void clearColorImage(VkImage image, VkImageLayout imageLayout,
                       VkClearColorValue const &color,
                       std::vector<VkImageSubresourceRange> const &ranges) {
    commandBuffer->vkCmdClearColorImage(*commandBuffer, image, imageLayout,
                                        &color, (uint32_t)ranges.size(),
                                        ranges.data());
  }

public:
  std::shared_ptr<CommandBuffer> commandBuffer;
};

struct RenderPassBeginInfo {
  std::shared_ptr<RenderPass> renderPass;
  std::shared_ptr<Framebuffer> framebuffer;
  VkRect2D renderArea;
  std::vector<VkClearValue> clearValues;
  VkSubpassContents subpassContents;
};

class CommandBufferRenderPass {
public:
  CommandBufferRenderPass(std::shared_ptr<CommandBufferRecording> recording,
                          RenderPassBeginInfo const &renderPassInfo) {
    this->commandBuffer = recording->commandBuffer;
    boundRefs.push_back(renderPassInfo.renderPass);
    boundRefs.push_back(recording);
    boundRefs.push_back(renderPassInfo.framebuffer);

    auto vk_renderPassInfo = VkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = *renderPassInfo.renderPass,
        .framebuffer = *renderPassInfo.framebuffer,
        .renderArea = renderPassInfo.renderArea,
        .clearValueCount = (uint32_t)renderPassInfo.clearValues.size(),
        .pClearValues = renderPassInfo.clearValues.data()

    };

    commandBuffer->vkCmdBeginRenderPass(*commandBuffer, &vk_renderPassInfo,
                                        renderPassInfo.subpassContents);
  }

  CommandBufferRenderPass(CommandBufferRenderPass const &) = delete;
  CommandBufferRenderPass(CommandBufferRenderPass &&) = delete;

  ~CommandBufferRenderPass() {
    commandBuffer->vkCmdEndRenderPass(*commandBuffer);
  }

  void bindPipeline(std::shared_ptr<GraphicsPipeline> pipeline) {
    boundRefs.push_back(pipeline);
    commandBuffer->vkCmdBindPipeline(
        *commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
  }

  void setViewport(VkViewport const &viewport) {
    commandBuffer->vkCmdSetViewport(*commandBuffer, 0, 1, &viewport);
  }

  void setScissor(VkRect2D const &scissor) {
    commandBuffer->vkCmdSetScissor(*commandBuffer, 0, 1, &scissor);
  }

  void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
            uint32_t firstInstance) {
    commandBuffer->vkCmdDraw(*commandBuffer, vertexCount, instanceCount,
                             firstVertex, firstInstance);
  }

private:
  std::shared_ptr<CommandBuffer> commandBuffer;
  std::vector<std::shared_ptr<void>> boundRefs;
};

class Semaphore {
public:
  Semaphore() = default;

  Semaphore(std::shared_ptr<Device> device) {
    this->device = device;
    auto vk_createInfo =
        VkSemaphoreCreateInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                              .pNext = nullptr,
                              .flags = {}};
    VK_CHECK(device->vkCreateSemaphore(*device, &vk_createInfo, nullptr,
                                       &semaphore));
  }

  Semaphore(Semaphore const &) = delete;
  Semaphore &operator=(Semaphore const &) = delete;

  Semaphore(Semaphore &&other) {
    *this = std::move(other);
  }

  Semaphore &operator=(Semaphore &&other) {
    this->~Semaphore();
    device = std::move(other.device);
    semaphore = std::move(other.semaphore);
    other.semaphore = VK_NULL_HANDLE;
    return *this;
  }

  ~Semaphore() {
    if (semaphore != VK_NULL_HANDLE)
      device->vkDestroySemaphore(*device, semaphore, nullptr);
    semaphore = VK_NULL_HANDLE;
  }

  operator VkSemaphore() {
    return semaphore;
  }

private:
  std::shared_ptr<Device> device = {};
  VkSemaphore semaphore = VK_NULL_HANDLE;
};

class Fence {
public:
  Fence() = default;

  Fence(std::shared_ptr<Device> device, bool signalled = false) {
    this->device = device;
    auto vk_createInfo =
        VkFenceCreateInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                          .pNext = nullptr,
                          .flags = signalled ? VK_FENCE_CREATE_SIGNALED_BIT
                                             : VkFenceCreateFlags{}};
    VK_CHECK(device->vkCreateFence(*device, &vk_createInfo, nullptr, &fence));
  }

  Fence(Fence const &) = delete;
  Fence &operator=(Fence const &) = delete;

  Fence(Fence &&other) {
    *this = std::move(other);
  }

  Fence &operator=(Fence &&other) {
    this->~Fence();
    device = std::move(other.device);
    fence = std::move(other.fence);
    other.fence = VK_NULL_HANDLE;
    return *this;
  }

  ~Fence() {
    if (fence != VK_NULL_HANDLE)
      device->vkDestroyFence(*device, fence, nullptr);
    fence = VK_NULL_HANDLE;
  }

  operator VkFence() {
    return fence;
  }

  void wait() {
    VK_CHECK(device->vkWaitForFences(*device, 1, &fence, VK_TRUE, UINT64_MAX));
  }

  void reset() {
    VK_CHECK(device->vkResetFences(*device, 1, &fence));
  }

private:
  std::shared_ptr<Device> device = {};
  VkFence fence = VK_NULL_HANDLE;
};

int main() {
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#endif

  auto onError = [](int errorCode, char const *description) {
    spdlog::error("GLFW error (Code: {}): \"{}\"", errorCode, description);
  };
  auto glfw = GLFWLib::getInstance(onError);

  auto window = Window(glfw, 800, 600, "vkt");

  window.onKey = [&](int key, int scanCode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE)
      glfwSetWindowShouldClose(window, GLFW_TRUE);
  };

  auto loader = std::make_shared<Loader>(glfwGetInstanceProcAddress);

  auto vkRootDebuggerLogger = spdlog::stdout_color_mt("Vk root debugger");

  std::vector<std::string> extensions = {"VK_KHR_portability_enumeration"};

  uint32_t numExtensions;
  auto extensionNames = glfwGetRequiredInstanceExtensions(&numExtensions);
  for (int idx = 0; idx < numExtensions; ++idx)
    extensions.push_back(extensionNames[idx]);

  auto instance = std::make_shared<Instance>(
      loader, ApplicationInfo{},
      InstanceCreateInfo{.flags =
                             VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
                         .enabledExtensions = extensions,
                         .enabledLayers = {"VK_LAYER_KHRONOS_validation"}},
      DebugMessengerCreateInfo{
          .severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
          .type = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
          .onLog = [&](VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                       VkDebugUtilsMessageTypeFlagsEXT types,
                       const VkDebugUtilsMessengerCallbackDataEXT *data) {
            spdlog::level::level_enum level;
            switch (severity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
              level = spdlog::level::err;
              break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
              level = spdlog::level::info;
              break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
              level = spdlog::level::debug;
              break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
              level = spdlog::level::warn;
              break;
            }

            vkRootDebuggerLogger->log(level, data->pMessage);
          }});

  auto vkDebuggerLogger = spdlog::stdout_color_mt("Vk debugger");

  auto debugMessenger = DebugMessenger(
      instance,
      DebugMessengerCreateInfo{
          .severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
          .type = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
          .onLog = [&](VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                       VkDebugUtilsMessageTypeFlagsEXT types,
                       const VkDebugUtilsMessengerCallbackDataEXT *data) {
            spdlog::level::level_enum level;
            switch (severity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
              level = spdlog::level::err;
              break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
              level = spdlog::level::info;
              break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
              level = spdlog::level::debug;
              break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
              level = spdlog::level::warn;
              break;
            }

            vkDebuggerLogger->log(level, data->pMessage);
          }});

  auto surface = Surface(instance, window);

  auto physicalDevices = instance->listPhysicalDevices();

  auto deviceInfo = [&](PhysicalDevice &physicalDevice) {
    uint32_t score = 0;
    bool isSuitable = true;

    switch (physicalDevice.properties.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      score += 1 << 10;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      score += 1 << 9;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      score += 1 << 8;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      score += 1 << 7;
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
      score += 1 << 6;
    };

    std::optional<uint32_t> graphicsQueueIndex, presentQueueIndex;

    for (uint32_t index = 0; index < physicalDevice.queueFamilies.size();
         ++index) {
      auto const &queueFamily = physicalDevice.queueFamilies[index];
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        graphicsQueueIndex = index;

      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface,
                                           &presentSupport);
      if (presentSupport)
        presentQueueIndex = index;
    }

    if (!graphicsQueueIndex.has_value() || !presentQueueIndex.has_value())
      isSuitable = false;

    return std::make_tuple(isSuitable, score, graphicsQueueIndex.value_or(0),
                           presentQueueIndex.value_or(0));
  };

  auto deviceScore = [&](PhysicalDevice &physicalDevice) {
    auto [isSuitable, score, _1, _2] = deviceInfo(physicalDevice);
    return std::make_tuple(isSuitable, score);
  };

  auto physicalDevice =
      *find_max(physicalDevices.begin(), physicalDevices.end(), deviceScore);
  auto [isSuitable, _, graphicsQueueIndex, presentQueueIndex] =
      deviceInfo(physicalDevice);

  if (!isSuitable)
    throw std::runtime_error("VK: No suitable devices found");

  std::vector<uint32_t> queueFamilyIndices;
  for (uint32_t index :
       std::set<uint32_t>{graphicsQueueIndex, presentQueueIndex})
    queueFamilyIndices.push_back(index);

  std::vector<DeviceQueueCreateInfo> queueCreateInfos;
  for (auto queueFamilyIndex : queueFamilyIndices)
    queueCreateInfos.emplace_back(DeviceQueueCreateInfo{
        .queueFamilyIndex = queueFamilyIndex, .queuePriorities = {1.0f}});
  auto device = std::make_shared<Device>(
      loader, physicalDevice,
      DeviceCreateInfo{.queueCreateInfos = queueCreateInfos,
                       .enabledExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME},
                       .enabledFeatures = physicalDevice.features});

  auto graphicsQueue = Queue(device, graphicsQueueIndex, 0),
       presentQueue = Queue(device, presentQueueIndex, 0);

  auto swapchainDetails = surface.getSwapchainDetails(physicalDevice);
  if (swapchainDetails.formats.empty() || swapchainDetails.presentModes.empty())
    throw std::runtime_error("VK: swapchain cannot be constructed");

  auto bestFormat = swapchainDetails.formats[0];
  for (auto const &format : swapchainDetails.formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      bestFormat = format;
      break;
    }
  }

  auto bestPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (auto const &presentMode : swapchainDetails.presentModes) {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      bestPresentMode = presentMode;
      break;
    }
  }

  VkExtent2D swapExtent;
  auto const &capabilities = swapchainDetails.capabilities;
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    swapExtent = capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    swapExtent = actualExtent;
  }

  auto swapchainInfo =
      SwapchainCreateInfo{.surface = surface,
                          .minImageCount = capabilities.minImageCount + 1,
                          .imageFormat = bestFormat.format,
                          .imageColorSpace = bestFormat.colorSpace,
                          .imageExtent = swapExtent,
                          .imageArrayLayers = 1,
                          .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                          .imageSharingMode = queueFamilyIndices.size() > 1
                                                  ? VK_SHARING_MODE_CONCURRENT
                                                  : VK_SHARING_MODE_EXCLUSIVE,
                          .queueFamilyIndices = queueFamilyIndices,
                          .preTransform = capabilities.currentTransform,
                          .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                          .presentMode = bestPresentMode,
                          .clipped = VK_TRUE,
                          .oldSwapchain = VK_NULL_HANDLE};

  auto swapchain = Swapchain(device, swapchainInfo);

  auto images = swapchain.getImages();
  std::vector<std::shared_ptr<ImageView>> imageViews;
  for (auto const &image : images) {
    imageViews.push_back(std::make_shared<ImageView>(
        device, ImageViewCreateInfo{
                    .image = image,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = swapchainInfo.imageFormat,
                    .components =
                        VkComponentMapping{.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                           .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                           .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                           .a = VK_COMPONENT_SWIZZLE_IDENTITY},
                    .subresourceRange = VkImageSubresourceRange{
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1}}));
  }

  ShaderModuleCreateInfo vertCreateInfo;
  {
    auto vertIfs = std::ifstream("shaders/triangle.vert.spv");
    vertCreateInfo.code = readFile(vertIfs);
  }
  auto vertShader = std::make_shared<ShaderModule>(device, vertCreateInfo);

  ShaderModuleCreateInfo fragCreateInfo;
  {
    auto fragIfs = std::ifstream("shaders/triangle.frag.spv");
    fragCreateInfo.code = readFile(fragIfs);
  }
  auto fragShader = std::make_shared<ShaderModule>(device, fragCreateInfo);

  auto pipelineLayout = std::make_shared<PipelineLayout>(
      device,
      PipelineLayoutCreateInfo{.setLayouts = {}, .pushConstantRanges = {}});

  auto renderPass = std::make_shared<RenderPass>(
      device,
      RenderPassCreateInfo{
          .attachments = {VkAttachmentDescription{
              .flags = {},
              .format = swapchainInfo.imageFormat,
              .samples = VK_SAMPLE_COUNT_1_BIT,
              .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
              .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
              .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
              .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
              .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
              .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR}},
          .subpasses = {SubpassDescription{
              .flags = {},
              .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
              .depthStencil =
                  VkAttachmentReference{.attachment = VK_ATTACHMENT_UNUSED,
                                        .layout = {}},
              .input = {},
              .color = {VkAttachmentReference{
                  .attachment = 0,
                  .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}},
              .resolve = {},
              .preserve = {}}},
          .dependencies = {VkSubpassDependency{
              .srcSubpass = VK_SUBPASS_EXTERNAL,
              .dstSubpass = 0,
              .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
              .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
              .srcAccessMask = 0,
              .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
              .dependencyFlags = {}}}});

  auto pipeline = std::make_shared<GraphicsPipeline>(
      device,
      GraphicsPipelineCreateInfo{
          .shaderStages =
              {ShaderStageCreateInfo{.stage = VK_SHADER_STAGE_VERTEX_BIT,
                                     .module = vertShader,
                                     .name = "main"},
               ShaderStageCreateInfo{.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                                     .module = fragShader,
                                     .name = "main"}},
          .vertexInputState = {.bindings = {}, .attributes = {}},
          .inputAssemblyState =
              {
                  .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                  .primitiveRestartEnable = VK_FALSE,
              },
          .viewportState = {.scissors = (uint32_t)1, .viewports = (uint32_t)1},
          .rasterizationState = {.depthClampEnable = VK_FALSE,
                                 .rasterizerDiscardEnable = VK_FALSE,
                                 .polygonMode = VK_POLYGON_MODE_FILL,
                                 .cullMode = VK_CULL_MODE_BACK_BIT,
                                 .frontFace = VK_FRONT_FACE_CLOCKWISE,
                                 .depthBiasEnable = VK_FALSE,
                                 .depthBiasConstantFactor = 0.0f,
                                 .depthBiasClamp = 0.0f,
                                 .depthBiasSlopeFactor = 0.0f,
                                 .lineWidth = 1.0f},
          .multisampleState = {.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                               .sampleShadingEnable = VK_FALSE,
                               .minSampleShading = 0.0f,
                               .sampleMask = std::nullopt,
                               .alphaToCoverageEnable = VK_FALSE,
                               .alphaToOneEnable = VK_FALSE},
          .colorBlendState =
              {.logicOpEnable = VK_FALSE,
               .logicOp = VK_LOGIC_OP_COPY,
               .attachments = {VkPipelineColorBlendAttachmentState{
                   .blendEnable = VK_FALSE,
                   .srcColorBlendFactor = {},
                   .dstColorBlendFactor = {},
                   .colorBlendOp = {},
                   .srcAlphaBlendFactor = {},
                   .dstAlphaBlendFactor = {},
                   .alphaBlendOp = {},
                   .colorWriteMask =
                       VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                       VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT}},
               .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}},
          .dynamicStates = {VK_DYNAMIC_STATE_SCISSOR,
                            VK_DYNAMIC_STATE_VIEWPORT},
          .pipelineLayout = pipelineLayout,
          .renderPass = renderPass,
          .subpass = 0});

  std::vector<std::shared_ptr<Framebuffer>> framebuffers;
  for (auto const &imageView : imageViews) {
    auto framebuffer = std::make_shared<Framebuffer>(
        device, FramebufferCreateInfo{.attachments = {imageView},
                                      .renderPass = renderPass,
                                      .extent = swapExtent,
                                      .layers = 1});
    framebuffers.push_back(framebuffer);
  }

  struct Frame {
    std::shared_ptr<CommandPool> commandPool;
    std::shared_ptr<CommandBuffer> commandBuffer;
    Fence inFlight;
    Semaphore imageAvailable, renderFinished;
  };

  std::vector<Frame> frames;
  for (int frameIndex = 0; frameIndex < 2; ++frameIndex) {
    auto commandPool = std::make_shared<CommandPool>(
        device, CommandPoolCreateInfo{
                    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                    .queueFamilyIndex = graphicsQueueIndex});

    auto commandBuffer = std::make_shared<CommandBuffer>(
        device,
        CommandBufferAllocateInfo{.commandPool = commandPool,
                                  .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY});

    frames.push_back({.commandPool = commandPool,
                      .commandBuffer = commandBuffer,
                      .inFlight = Fence(device, true),
                      .imageAvailable = Semaphore(device),
                      .renderFinished = Semaphore(device)});
  }

  int curFrameIndex = 0;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    auto &frame = frames[curFrameIndex];

    frame.inFlight.wait();
    frame.inFlight.reset();

    auto imageIndex =
        swapchain.acquireNextImage(frame.imageAvailable, VK_NULL_HANDLE);
    auto const &framebuffer = framebuffers[imageIndex];

    frame.commandBuffer->reset();

    {
      auto cmdBufRecording = std::make_shared<CommandBufferRecording>(
          frame.commandBuffer, CommandBufferBeginInfo{.flags = {}});

      {
        auto cmdBufRenderPass = CommandBufferRenderPass(
            cmdBufRecording,
            RenderPassBeginInfo{
                .renderPass = renderPass,
                .framebuffer = framebuffer,
                .renderArea = VkRect2D{.offset = {0, 0}, .extent = swapExtent},
                .clearValues = {VkClearValue{
                    .color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}}},
                .subpassContents = VK_SUBPASS_CONTENTS_INLINE});

        cmdBufRenderPass.bindPipeline(pipeline);

        cmdBufRenderPass.setViewport(
            VkViewport{.x = 0.0f,
                       .y = 0.0f,
                       .width = (float)swapExtent.width,
                       .height = (float)swapExtent.height,
                       .minDepth = 0.0f,
                       .maxDepth = 1.0f});

        cmdBufRenderPass.setScissor(
            VkRect2D{.offset = {0, 0}, .extent = swapExtent});

        cmdBufRenderPass.draw(3, 1, 0, 0);
      }
    }

    graphicsQueue.submit({.waitSemaphoresAndStages =
                              {{(VkSemaphore)frame.imageAvailable,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}},
                          .commandBuffers = {*frame.commandBuffer},
                          .signalSemaphores = {frame.renderFinished},
                          .fence = frame.inFlight});

    presentQueue.present({.waitSemaphores = {frame.renderFinished},
                          .swapchainsAndImageIndices = {
                              {(VkSwapchainKHR)swapchain, imageIndex}}});

    curFrameIndex = (curFrameIndex + 1) % frames.size();
  }

  vkDeviceWaitIdle(*device);

  return EXIT_SUCCESS;
}