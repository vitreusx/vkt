#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <set>
#include <fstream>
#include <chrono>
#include <glm/glm.hpp>
#include <vkt/vkt.h>

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription binding(uint32_t bindingIndex = 0) {
    return {.binding = bindingIndex,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
  }

  static std::vector<VkVertexInputAttributeDescription>
  attributes(uint32_t bindingIndex = 0) {
    return {
        VkVertexInputAttributeDescription{.location = 0,
                                          .binding = bindingIndex,
                                          .format = VK_FORMAT_R32G32_SFLOAT,
                                          .offset = offsetof(Vertex, pos)},
        VkVertexInputAttributeDescription{.location = 1,
                                          .binding = bindingIndex,
                                          .format = VK_FORMAT_R32G32B32_SFLOAT,
                                          .offset = offsetof(Vertex, color)}};
  }
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

  struct DeviceInfo {
    bool isSuitable;
    uint32_t score;
    uint32_t graphicsQueueIndex;
    uint32_t presentQueueIndex;
    uint32_t transferQueueIndex;
  };

  auto getDeviceInfo = [&](PhysicalDevice &physicalDevice) {
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

    std::optional<uint32_t> graphicsQueueIndex, presentQueueIndex,
        transferQueueIndex;

    for (uint32_t index = 0; index < physicalDevice.queueFamilies.size();
         ++index) {
      auto const &queueFamily = physicalDevice.queueFamilies[index];
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        graphicsQueueIndex = index;
      else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
        transferQueueIndex = index;

      VkBool32 presentSupport = false;
      loader->vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index,
                                                   surface, &presentSupport);
      if (presentSupport)
        presentQueueIndex = index;
    }

    if (!transferQueueIndex.has_value())
      transferQueueIndex = graphicsQueueIndex;

    if (!graphicsQueueIndex.has_value() || !presentQueueIndex.has_value() ||
        !transferQueueIndex.has_value())
      isSuitable = false;

    return DeviceInfo{.isSuitable = isSuitable,
                      .score = score,
                      .graphicsQueueIndex = graphicsQueueIndex.value_or(0),
                      .presentQueueIndex = presentQueueIndex.value_or(0),
                      .transferQueueIndex = transferQueueIndex.value_or(0)};
  };

  auto deviceScore = [&](PhysicalDevice &physicalDevice) {
    auto deviceInfo = getDeviceInfo(physicalDevice);
    return std::make_tuple(deviceInfo.isSuitable, deviceInfo.score);
  };

  auto physicalDevice =
      *find_max(physicalDevices.begin(), physicalDevices.end(), deviceScore);
  auto deviceInfo = getDeviceInfo(physicalDevice);

  if (!deviceInfo.isSuitable)
    throw std::runtime_error("VK: No suitable devices found");

  std::vector<uint32_t> queueFamilyIndices;
  for (uint32_t index : std::set<uint32_t>{deviceInfo.graphicsQueueIndex,
                                           deviceInfo.presentQueueIndex,
                                           deviceInfo.transferQueueIndex})
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

  auto graphicsQueue = Queue(device, deviceInfo.graphicsQueueIndex, 0),
       presentQueue = Queue(device, deviceInfo.presentQueueIndex, 0),
       transferQueue = Queue(device, deviceInfo.transferQueueIndex, 0);

  bool framebufferResized = false;
  window.onFramebufferSize = [&](int width, int height) -> void {
    framebufferResized = true;
  };

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

  Swapchain swapchain;
  std::vector<VkImage> images;
  std::vector<std::shared_ptr<ImageView>> imageViews;
  VkExtent2D swapExtent;
  SwapchainCreateInfo swapchainInfo;
  std::vector<std::shared_ptr<Framebuffer>> framebuffers;

  auto createSwapchain = [&]() -> void {
    vkDeviceWaitIdle(*device);

    auto swapchainDetails = surface.getSwapchainDetails(physicalDevice);
    if (swapchainDetails.formats.empty() ||
        swapchainDetails.presentModes.empty())
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
    auto const &capabilities = swapchainDetails.capabilities;
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
      swapExtent = capabilities.currentExtent;
    } else {
      int width = 0, height = 0;
      glfwGetFramebufferSize(window, &width, &height);

      while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
      }

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

    swapchainInfo =
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

    swapchain = Swapchain(device, swapchainInfo);

    images = swapchain.getImages();

    imageViews = {};
    for (auto const &image : images) {
      imageViews.push_back(std::make_shared<ImageView>(
          device,
          ImageViewCreateInfo{
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
  };

  createSwapchain();

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

  auto vertices = std::vector<Vertex>{{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                      {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                      {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                      {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
  auto verticesSize = sizeof(Vertex) * vertices.size();

  auto indices = std::vector<uint32_t>{0, 1, 2, 2, 3, 0};
  auto indicesSize = sizeof(uint32_t) * indices.size();

  auto findMemoryType = [&](uint32_t memoryTypeMask,
                            VkMemoryPropertyFlags propertyFlags) {
    for (uint32_t index = 0; index < physicalDevice.memoryProps.memoryTypeCount;
         ++index) {
      auto const &memoryType = physicalDevice.memoryProps.memoryTypes[index];
      if (memoryTypeMask & ((uint32_t)1 << index) == 0)
        continue;
      if ((memoryType.propertyFlags & propertyFlags) != propertyFlags)
        continue;
      return index;
    }
    throw std::runtime_error("Failed to find suitable memory type.");
  };

  auto vertexBuffer = std::make_shared<Buffer>(
      device, BufferCreateInfo{
                  .size = verticesSize,
                  .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  .sharingMode = VK_SHARING_MODE_CONCURRENT,
                  .queueFamilyIndices = {deviceInfo.graphicsQueueIndex,
                                         deviceInfo.transferQueueIndex},
              });
  auto vbMemoryReqs = vertexBuffer->getMemoryRequirements();

  auto vbMemory = DeviceMemory(
      device, MemoryAllocateInfo{.size = vbMemoryReqs.size,
                                 .memoryTypeIndex = findMemoryType(
                                     vbMemoryReqs.memoryTypeBits,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)});
  VK_CHECK(device->vkBindBufferMemory(*device, *vertexBuffer, vbMemory, 0));

  auto indexBuffer = std::make_shared<Buffer>(
      device,
      BufferCreateInfo{.size = indicesSize,
                       .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       .sharingMode = VK_SHARING_MODE_CONCURRENT,
                       .queueFamilyIndices = {deviceInfo.graphicsQueueIndex,
                                              deviceInfo.transferQueueIndex}});
  auto ibMemoryReqs = indexBuffer->getMemoryRequirements();

  auto ibMemory = DeviceMemory(
      device, MemoryAllocateInfo{.size = ibMemoryReqs.size,
                                 .memoryTypeIndex = findMemoryType(
                                     ibMemoryReqs.memoryTypeBits,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)});
  VK_CHECK(device->vkBindBufferMemory(*device, *indexBuffer, ibMemory, 0));

  {
    auto stagingBuffer = std::make_shared<Buffer>(
        device, BufferCreateInfo{
                    .size = verticesSize,
                    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndices = {deviceInfo.transferQueueIndex}});
    auto stagingMemoryReqs = stagingBuffer->getMemoryRequirements();

    auto stagingMemory = DeviceMemory(
        device,
        MemoryAllocateInfo{.size = stagingMemoryReqs.size,
                           .memoryTypeIndex = findMemoryType(
                               stagingMemoryReqs.memoryTypeBits,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)});
    VK_CHECK(
        device->vkBindBufferMemory(*device, *stagingBuffer, stagingMemory, 0));

    auto stagingMemoryMap = stagingMemory.map();
    std::memcpy(stagingMemoryMap.get(), vertices.data(), verticesSize);

    auto commandBuffer = std::make_shared<CommandBuffer>(
        device, CommandBufferAllocateInfo{
                    .commandPool = std::make_shared<CommandPool>(
                        device,
                        CommandPoolCreateInfo{
                            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                            .queueFamilyIndex = deviceInfo.transferQueueIndex}),
                    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY});

    {
      auto recording = CommandBufferRecording(
          commandBuffer,
          CommandBufferBeginInfo{
              .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT});

      recording.copyBuffer(*stagingBuffer, *vertexBuffer,
                           {VkBufferCopy{.size = verticesSize}});
    }

    transferQueue.submit(QueueSubmitInfo{
        .waitSemaphoresAndStages = {},
        .commandBuffers = {(VkCommandBuffer)*commandBuffer},
        .signalSemaphores = {},
        .fence = VK_NULL_HANDLE,
    });
    transferQueue.wait();
  }

  {
    auto stagingBuffer = std::make_shared<Buffer>(
        device, BufferCreateInfo{
                    .size = indicesSize,
                    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndices = {deviceInfo.transferQueueIndex}});
    auto stagingMemoryReqs = stagingBuffer->getMemoryRequirements();

    auto stagingMemory = DeviceMemory(
        device,
        MemoryAllocateInfo{.size = stagingMemoryReqs.size,
                           .memoryTypeIndex = findMemoryType(
                               stagingMemoryReqs.memoryTypeBits,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)});
    VK_CHECK(
        device->vkBindBufferMemory(*device, *stagingBuffer, stagingMemory, 0));

    auto stagingMemoryMap = stagingMemory.map();
    std::memcpy(stagingMemoryMap.get(), indices.data(), indicesSize);

    auto commandBuffer = std::make_shared<CommandBuffer>(
        device, CommandBufferAllocateInfo{
                    .commandPool = std::make_shared<CommandPool>(
                        device,
                        CommandPoolCreateInfo{
                            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                            .queueFamilyIndex = deviceInfo.transferQueueIndex}),
                    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY});

    {
      auto recording = CommandBufferRecording(
          commandBuffer,
          CommandBufferBeginInfo{
              .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT});

      recording.copyBuffer(*stagingBuffer, *indexBuffer,
                           {VkBufferCopy{.size = indicesSize}});
    }

    transferQueue.submit(QueueSubmitInfo{
        .waitSemaphoresAndStages = {},
        .commandBuffers = {(VkCommandBuffer)*commandBuffer},
        .signalSemaphores = {},
        .fence = VK_NULL_HANDLE,
    });
    transferQueue.wait();
  }

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
          .vertexInputState = {.bindings = {Vertex::binding()},
                               .attributes = Vertex::attributes()},
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

  auto createFramebuffers = [&]() -> void {
    framebuffers = {};
    for (auto const &imageView : imageViews) {
      auto framebuffer = std::make_shared<Framebuffer>(
          device, FramebufferCreateInfo{.attachments = {imageView},
                                        .renderPass = renderPass,
                                        .extent = swapExtent,
                                        .layers = 1});
      framebuffers.push_back(framebuffer);
    }
  };

  createFramebuffers();

  auto recreateSwapchain = [&]() -> void {
    createSwapchain();
    createFramebuffers();
  };

  struct Frame {
    std::shared_ptr<CommandBuffer> commandBuffer;
    Fence inFlight;
    Semaphore imageAvailable, renderFinished;
  };

  std::vector<Frame> frames;
  for (int frameIndex = 0; frameIndex < 2; ++frameIndex) {
    auto commandBuffer = std::make_shared<CommandBuffer>(
        device,
        CommandBufferAllocateInfo{
            .commandPool = std::make_shared<CommandPool>(
                device,
                CommandPoolCreateInfo{
                    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                    .queueFamilyIndex = deviceInfo.graphicsQueueIndex}),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY});

    frames.push_back({.commandBuffer = commandBuffer,
                      .inFlight = Fence(device, true),
                      .imageAvailable = Semaphore(device),
                      .renderFinished = Semaphore(device)});
  }

  int curFrameIndex = 0;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    auto &frame = frames[curFrameIndex];

    frame.inFlight.wait();

    auto [imageIndex, acqResult] =
        swapchain.acquireNextImage(frame.imageAvailable, VK_NULL_HANDLE);

    if (acqResult == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapchain();
      continue;
    } else if (acqResult != VK_SUCCESS && acqResult != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("vkAcquireNextImageKHR");
    }

    frame.inFlight.reset();

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

        cmdBufRenderPass.bindVertexBuffers({{vertexBuffer, (VkDeviceSize)0}});

        cmdBufRenderPass.bindIndexBuffer(*indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        cmdBufRenderPass.setViewport(
            VkViewport{.x = 0.0f,
                       .y = 0.0f,
                       .width = (float)swapExtent.width,
                       .height = (float)swapExtent.height,
                       .minDepth = 0.0f,
                       .maxDepth = 1.0f});

        cmdBufRenderPass.setScissor(
            VkRect2D{.offset = {0, 0}, .extent = swapExtent});

        cmdBufRenderPass.drawIndexed(indices.size(), 1, 0, 0, 0);
      }
    }

    graphicsQueue.submit({.waitSemaphoresAndStages =
                              {{(VkSemaphore)frame.imageAvailable,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}},
                          .commandBuffers = {*frame.commandBuffer},
                          .signalSemaphores = {frame.renderFinished},
                          .fence = frame.inFlight});

    auto presResult =
        presentQueue.present({.waitSemaphores = {frame.renderFinished},
                              .swapchainsAndImageIndices = {
                                  {(VkSwapchainKHR)swapchain, imageIndex}}});

    if (presResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presResult == VK_SUBOPTIMAL_KHR || framebufferResized) {
      framebufferResized = false;
      recreateSwapchain();
    } else if (presResult != VK_SUCCESS) {
      throw std::runtime_error("vkPresentQueueKHR");
    }

    curFrameIndex = (curFrameIndex + 1) % frames.size();
  }

  device->vkDeviceWaitIdle(*device);

  return EXIT_SUCCESS;
}