#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <set>
#include <fstream>
#include <chrono>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vkt/vkt.h>
#include <vkx/obj_model.h>
#include <vkx/stb_image.h>
#include <map>

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
  glm::vec2 texCoord;
};

struct VSUnif {
  alignas(16) glm::mat4 mvp;
};

struct MaterialData {
  alignas(16) glm::vec3 ambient, diffuse, specular, transmittance, emission;
  alignas(16) float shininess, ior, dissolve;
  alignas(16) int illum;
};

class TriangleShaders {
public:
  TriangleShaders(std::shared_ptr<Device> device, std::string const &vsSource,
                  std::string const &fsSource) {
    this->device = device;
    vertShader = std::make_shared<ShaderModule>(
        device, ShaderModuleCreateInfo{.code = vsSource});
    fragShader = std::make_shared<ShaderModule>(
        device, ShaderModuleCreateInfo{.code = fsSource});

    vertexInputState = {
        .bindings = {{.binding = 0,
                      .stride = sizeof(Vertex),
                      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}},
        .attributes = {{.location = 0,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                        .offset = offsetof(Vertex, pos)},
                       {.location = 1,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                        .offset = offsetof(Vertex, color)},
                       {.location = 2,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                        .offset = offsetof(Vertex, normal)},
                       {.location = 3,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32_SFLOAT,
                        .offset = offsetof(Vertex, texCoord)}}};

    perFrame = {{.binding = 0,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                 .pImmutableSamplers = nullptr}};

    perMaterial = {{.binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr}};

    for (uint32_t binding = 1; binding <= 8; ++binding) {
      perMaterial.push_back(VkDescriptorSetLayoutBinding{
          .binding = binding,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .pImmutableSamplers = nullptr});
    }

    perFrameLayout = DescriptorSetLayout(device, {.bindings = perFrame});
    perMaterialLayout = DescriptorSetLayout(device, {.bindings = perMaterial});

    pipelineLayout = std::make_shared<PipelineLayout>(
        device, PipelineLayoutCreateInfo{
                    .setLayouts = {perFrameLayout, perMaterialLayout},
                    .pushConstantRanges = {}});

    shaderStages = {{.stage = VK_SHADER_STAGE_VERTEX_BIT,
                     .module = vertShader,
                     .name = "main"},
                    {.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                     .module = fragShader,
                     .name = "main"}};
  }

  DescriptorPool createDescriptorPool(
      std::vector<VkDescriptorSetLayoutBinding> const &bindings,
      uint32_t setCount) {
    std::map<VkDescriptorType, uint32_t> poolSizes;
    for (auto const &binding : bindings) {
      if (poolSizes.find(binding.descriptorType) == poolSizes.end())
        poolSizes[binding.descriptorType] = 0;
      poolSizes[binding.descriptorType] += setCount * binding.descriptorCount;
    }

    DescriptorPoolCreateInfo createInfo;
    createInfo.maxSets = setCount;
    for (auto const &[descriptorType, totalDescriptorCount] : poolSizes)
      createInfo.poolSizes.push_back(
          {.type = descriptorType, .descriptorCount = totalDescriptorCount});

    return DescriptorPool(device, createInfo);
  }

  std::vector<VkDescriptorSet> createPerFrameSets(uint32_t count) {
    auto &pool =
        descriptorPools.emplace_back(createDescriptorPool(perFrame, count));
    return pool.allocateDescriptorSets(
        {.setLayouts =
             std::vector<VkDescriptorSetLayout>(count, perFrameLayout)});
  }

  std::vector<VkDescriptorSet> createPerMaterialSets(uint32_t count) {
    auto &pool =
        descriptorPools.emplace_back(createDescriptorPool(perMaterial, count));
    return pool.allocateDescriptorSets(
        {.setLayouts =
             std::vector<VkDescriptorSetLayout>(count, perMaterialLayout)});
  }

  std::shared_ptr<Device> device;
  std::shared_ptr<ShaderModule> vertShader, fragShader;
  VertexInputStateCreateInfo vertexInputState;
  VkVertexInputBindingDescription inputBinding;
  std::vector<VkVertexInputAttributeDescription> inputAttributes;
  std::vector<VkDescriptorSetLayoutBinding> perFrame, perMaterial;
  DescriptorSetLayout perFrameLayout, perMaterialLayout;
  std::shared_ptr<PipelineLayout> pipelineLayout;
  std::vector<DescriptorPool> descriptorPools;
  std::vector<ShaderStageCreateInfo> shaderStages;
};

class Texture {
public:
  Texture() = default;

  Texture(std::shared_ptr<Device> device, std::filesystem::path const &path) {}
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

  auto loader = std::make_shared<Loader>(glfwGetInstanceProcAddress);

  auto vkRootDebuggerLogger = spdlog::stdout_color_mt("Vk root debugger");

  std::vector<std::string> extensions = {
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};

  uint32_t numExtensions;
  auto extensionNames = glfwGetRequiredInstanceExtensions(&numExtensions);
  for (int idx = 0; idx < numExtensions; ++idx)
    extensions.push_back(extensionNames[idx]);

  auto instance = std::make_shared<Instance>(
      loader, ApplicationInfo{.apiVersion = {.major = 1, .minor = 3}},
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

    if (physicalDevice.features.samplerAnisotropy != VK_TRUE)
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

  uint32_t maxFramesInFlight = 2;

  std::string vsSource, fsSource;
  {
    std::ifstream vsFile("shaders/triangle.vert.spv"),
        fsFile("shaders/triangle.frag.spv");
    vsSource = readFile(vsFile);
    fsSource = readFile(fsFile);
  }

  auto triangleShaders = TriangleShaders(device, vsSource, fsSource);
  auto pipelineLayout = triangleShaders.pipelineLayout;

  VSUnif vsUnif;
  auto perFrameSets = triangleShaders.createPerFrameSets(maxFramesInFlight);

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

  VkFormat depthFormat =
      physicalDevice
          .findSuitableFormat(
              {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
          .value();

  auto depthImage = Image(
      device,
      ImageCreateInfo{.flags = {},
                      .imageType = VK_IMAGE_TYPE_2D,
                      .format = depthFormat,
                      .extent = VkExtent3D{.width = swapExtent.width,
                                           .height = swapExtent.height,
                                           .depth = 1},
                      .mipLevels = 1,
                      .arrayLayers = 1,
                      .samples = VK_SAMPLE_COUNT_1_BIT,
                      .tiling = VK_IMAGE_TILING_OPTIMAL,
                      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                      .queueFamilyIndices = {deviceInfo.graphicsQueueIndex},
                      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED});

  auto depthImageMemory =
      depthImage.allocMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  auto depthImageView = std::make_shared<ImageView>(
      device,
      ImageViewCreateInfo{
          .image = depthImage,
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = depthFormat,
          .components = VkComponentMapping{.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                           .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                           .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                           .a = VK_COMPONENT_SWIZZLE_IDENTITY},
          .subresourceRange =
              VkImageSubresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT,
                                      .baseMipLevel = 0,
                                      .levelCount = 1,
                                      .baseArrayLayer = 0,
                                      .layerCount = 1}});

  auto renderPass = std::make_shared<RenderPass>(
      device,
      RenderPassCreateInfo{
          .attachments =
              {VkAttachmentDescription{
                   .flags = {},
                   .format = swapchainInfo.imageFormat,
                   .samples = VK_SAMPLE_COUNT_1_BIT,
                   .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                   .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                   .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                   .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                   .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                   .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR},
               VkAttachmentDescription{
                   .flags = {},
                   .format = depthFormat,
                   .samples = VK_SAMPLE_COUNT_1_BIT,
                   .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                   .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                   .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                   .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                   .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                   .finalLayout =
                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}},
          .subpasses = {SubpassDescription{
              .flags = {},
              .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
              .depthStencil =
                  VkAttachmentReference{
                      .attachment = 1,
                      .layout =
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
              .input = {},
              .color = {VkAttachmentReference{
                  .attachment = 0,
                  .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}},
              .resolve = {},
              .preserve = {}}},
          .dependencies = {VkSubpassDependency{
              .srcSubpass = VK_SUBPASS_EXTERNAL,
              .dstSubpass = 0,
              .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
              .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
              .srcAccessMask = 0,
              .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
              .dependencyFlags = {}}}});

  auto model = ObjModel("assets/sibenik/sibenik.obj");
  auto const &mesh = model.shapes[0].mesh;

  uint32_t numBaseVertices = model.attrib.vertices.size() / 3;
  std::vector<Vertex> baseVertices(numBaseVertices);
  for (int idx = 0; idx < numBaseVertices; ++idx) {
    baseVertices[idx] =
        Vertex{.pos = {model.attrib.vertices[3 * idx],
                       model.attrib.vertices[3 * idx + 1],
                       model.attrib.vertices[3 * idx + 2]},
               .color = {model.attrib.colors[3 * idx],
                         model.attrib.colors[3 * idx + 1],
                         model.attrib.colors[3 * idx + 2]},
               .texCoord = {model.attrib.texcoords[2 * idx],
                            model.attrib.texcoords[2 * idx + 1]}};
  }

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  std::map<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t> vertMap;

  for (auto &index : mesh.indices) {
    uint32_t v = std::max(index.vertex_index, 0),
             n = std::max(index.normal_index, 0),
             t = std::max(index.texcoord_index, 0);

    auto mask = std::make_tuple(v, n, t);

    uint32_t realIndex;
    auto viter = vertMap.find(mask);

    if (viter == vertMap.end()) {
      realIndex = vertices.size();
      vertices.push_back(Vertex{.pos = baseVertices[v].pos,
                                .color = baseVertices[v].color,
                                .normal = baseVertices[n].normal,
                                .texCoord = baseVertices[t].texCoord});
      vertMap[mask] = realIndex;
    } else {
      realIndex = viter->second;
    }

    indices.push_back(realIndex);
  }

  auto verticesSize = sizeof(Vertex) * vertices.size();
  auto indicesSize = sizeof(uint32_t) * indices.size();

  struct Submesh {
    uint32_t offset, count, mat;
  };

  std::vector<Submesh> submeshes;
  uint32_t curStart = 0, curMaterial = mesh.material_ids[0];
  for (uint32_t face = 1; face < mesh.material_ids.size(); ++face) {
    if (mesh.material_ids[face] != curMaterial) {
      submeshes.push_back(
          Submesh{3 * curStart, 3 * (face - curStart), curMaterial});
      curStart = face;
      curMaterial = mesh.material_ids[face];
    }
  }
  if (curStart < mesh.material_ids.size())
    submeshes.push_back(Submesh{
        3 * curStart, 3 * ((uint32_t)mesh.material_ids.size() - curStart),
        curMaterial});

  std::set<std::string> texNames = {""};
  for (auto const &material : model.materials) {
    if (!material.ambient_texname.empty())
      texNames.insert(material.ambient_texname);
    if (!material.diffuse_texname.empty())
      texNames.insert(material.diffuse_texname);
    if (!material.specular_texname.empty())
      texNames.insert(material.specular_texname);
    if (!material.emissive_texname.empty())
      texNames.insert(material.emissive_texname);
    if (!material.bump_texname.empty())
      texNames.insert(material.bump_texname);
  }

  struct Texture {
    Image image;
    DeviceMemory memory;
    ImageView imageView;
    Sampler sampler;
    VkImageLayout imageLayout;
    uint32_t size;
  };

  std::map<std::string, Texture> textures;
  for (auto texName : texNames) {
    auto &tex = textures[texName];

    std::filesystem::path texPath;
    for (auto &c : texName)
      if (c == '\\')
        c = '/';

    if (!texName.empty())
      texPath = model.root / texName;
    else
      texPath = "assets/missing.png";

    auto imageFile = StbImage(texPath, STBI_rgb_alpha);

    auto texFormat =
        physicalDevice
            .findSuitableFormat({VK_FORMAT_R8G8B8A8_SRGB},
                                VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
            .value();

    auto imageExtent = VkExtent3D{.width = (uint32_t)imageFile.width,
                                  .height = (uint32_t)imageFile.height,
                                  .depth = 1};

    tex.image = Image(
        device,
        ImageCreateInfo{.imageType = VK_IMAGE_TYPE_2D,
                        .format = texFormat,
                        .extent = imageExtent,
                        .mipLevels = 1,
                        .arrayLayers = 1,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .tiling = VK_IMAGE_TILING_OPTIMAL,
                        .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                        .sharingMode = VK_SHARING_MODE_CONCURRENT,
                        .queueFamilyIndices = {deviceInfo.graphicsQueueIndex,
                                               deviceInfo.transferQueueIndex},
                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED});

    tex.memory = tex.image.allocMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    tex.imageView = ImageView(
        device, ImageViewCreateInfo{
                    .image = tex.image,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = texFormat,
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
                        .layerCount = 1}});

    tex.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    tex.image.stage(imageFile.data, imageFile.nbytes(), transferQueue,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_ACCESS_SHADER_READ_BIT, tex.imageLayout);

    tex.sampler = Sampler(
        device, VkSamplerCreateInfo{
                    .magFilter = VK_FILTER_LINEAR,
                    .minFilter = VK_FILTER_LINEAR,
                    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                    .mipLodBias = 0.0f,
                    .anisotropyEnable = VK_TRUE,
                    .maxAnisotropy =
                        physicalDevice.properties.limits.maxSamplerAnisotropy,
                    .compareEnable = VK_FALSE,
                    .compareOp = VK_COMPARE_OP_ALWAYS,
                    .minLod = 0.0f,
                    .maxLod = 0.0f,
                    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                    .unnormalizedCoordinates = VK_FALSE});
  }

  uint32_t numMaterials = model.materials.size();
  auto perMaterialSets =
      triangleShaders.createPerMaterialSets(maxFramesInFlight * numMaterials);

  struct Material {
    Buffer dataBuf;
    DeviceMemory dataBufMemory;
    std::vector<std::string> texNames;
  };
  std::vector<Material> vkMaterials;

  for (auto &mat : model.materials) {
    auto &vkMat = vkMaterials.emplace_back();

    auto matData = MaterialData{
        .ambient = {mat.ambient[0], mat.ambient[1], mat.ambient[2]},
        .diffuse = {mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]},
        .specular = {mat.specular[0], mat.specular[1], mat.specular[2]},
        .transmittance = {mat.transmittance[0], mat.transmittance[1],
                          mat.transmittance[2]},
        .emission = {mat.emission[0], mat.emission[1], mat.emission[2]},
        .shininess = mat.shininess,
        .ior = mat.ior,
        .dissolve = mat.dissolve,
        .illum = mat.illum};

    vkMat.dataBuf =
        Buffer(device, {.size = sizeof(MaterialData),
                        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        .sharingMode = VK_SHARING_MODE_CONCURRENT,
                        .queueFamilyIndices = {deviceInfo.graphicsQueueIndex,
                                               deviceInfo.transferQueueIndex}});
    vkMat.dataBufMemory =
        vkMat.dataBuf.allocMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkMat.dataBuf.stage(&vkMat, sizeof(vkMat), transferQueue);

    vkMat.texNames = {mat.ambient_texname,  mat.diffuse_texname,
                      mat.specular_texname, mat.specular_highlight_texname,
                      mat.bump_texname,     mat.displacement_texname,
                      mat.alpha_texname,    mat.reflection_texname};
  }

  auto vertexBuffer = std::make_shared<Buffer>(
      device, BufferCreateInfo{
                  .size = verticesSize,
                  .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  .sharingMode = VK_SHARING_MODE_CONCURRENT,
                  .queueFamilyIndices = {deviceInfo.graphicsQueueIndex,
                                         deviceInfo.transferQueueIndex},
              });

  auto vertexBufferMemory =
      vertexBuffer->allocMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vertexBuffer->stage(vertices.data(), vertices.size() * sizeof(vertices[0]),
                      transferQueue);

  auto indexBuffer = std::make_shared<Buffer>(
      device,
      BufferCreateInfo{.size = indicesSize,
                       .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       .sharingMode = VK_SHARING_MODE_CONCURRENT,
                       .queueFamilyIndices = {deviceInfo.graphicsQueueIndex,
                                              deviceInfo.transferQueueIndex}});

  auto indexBufferMemory =
      indexBuffer->allocMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  indexBuffer->stage(indices.data(), indices.size() * sizeof(indices[0]),
                     transferQueue);

  auto pipeline = std::make_shared<GraphicsPipeline>(
      device,
      GraphicsPipelineCreateInfo{
          .shaderStages = triangleShaders.shaderStages,
          .vertexInputState = triangleShaders.vertexInputState,
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
          .depthStencilState = {.depthTestEnable = VK_TRUE,
                                .depthWriteEnable = VK_TRUE,
                                .depthCompareOp = VK_COMPARE_OP_LESS,
                                .depthBoundsTestEnable = VK_FALSE,
                                .stencilTestEnable = VK_FALSE},
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
          device,
          FramebufferCreateInfo{.attachments = {imageView, depthImageView},
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
    std::shared_ptr<Buffer> unifVsBuffer;
    std::shared_ptr<DeviceMemory> unifVsMemory;
    DeviceMemoryMap unifVsPtr;
    VkDescriptorSet perFrameSet;
    std::vector<VkDescriptorSet> perMaterialSets;
  };

  std::vector<Frame> frames;
  std::vector<DescriptorOp> descriptorOps;
  for (int frameIndex = 0; frameIndex < maxFramesInFlight; ++frameIndex) {
    auto &frame = frames.emplace_back();

    frame.commandBuffer = std::make_shared<CommandBuffer>(
        device,
        CommandBufferAllocateInfo{
            .commandPool = std::make_shared<CommandPool>(
                device,
                CommandPoolCreateInfo{
                    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                    .queueFamilyIndex = deviceInfo.graphicsQueueIndex}),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY});

    frame.unifVsBuffer = std::make_shared<Buffer>(
        device, BufferCreateInfo{
                    .size = sizeof(VSUnif),
                    .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndices = {deviceInfo.graphicsQueueIndex}});

    frame.unifVsMemory = std::make_shared<DeviceMemory>(
        frame.unifVsBuffer->allocMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    frame.unifVsPtr = DeviceMemoryMap(frame.unifVsMemory);

    frame.perFrameSet = perFrameSets[frameIndex];
    descriptorOps.push_back(WriteDescriptorSet{
        .dstSet = frame.perFrameSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .bufferInfos = {VkDescriptorBufferInfo{.buffer = *frame.unifVsBuffer,
                                               .offset = 0,
                                               .range = sizeof(VSUnif)}}});

    for (uint32_t matIdx = 0; matIdx < numMaterials; ++matIdx) {
      auto matSet = perMaterialSets[frameIndex * numMaterials + matIdx];
      frame.perMaterialSets.push_back(matSet);

      auto &vkMat = vkMaterials[matIdx];

      uint32_t curBinding = 0;

      descriptorOps.push_back(WriteDescriptorSet{
          .dstSet = matSet,
          .dstBinding = (curBinding++),
          .dstArrayElement = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .bufferInfos = {
              VkDescriptorBufferInfo{.buffer = vkMat.dataBuf,
                                     .offset = 0,
                                     .range = sizeof(MaterialData)}}});

      for (auto const &texName : vkMat.texNames) {
        auto &tex = textures[texName];
        descriptorOps.push_back(WriteDescriptorSet{
            .dstSet = matSet,
            .dstBinding = (curBinding++),
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .imageInfos = {
                VkDescriptorImageInfo{.sampler = tex.sampler,
                                      .imageView = tex.imageView,
                                      .imageLayout = tex.imageLayout}}});
      }
    }

    frame.inFlight = Fence(device, true);
    frame.imageAvailable = Semaphore(device);
    frame.renderFinished = Semaphore(device);
  }

  device->updateDescriptorSets(descriptorOps);
  descriptorOps.clear();

  glm::vec3 cameraPos = {2.0f, 2.0f, 2.0f};
  glm::vec<3, int> velocity = {0, 0, 0};
  float speed = 2.0f;
  bool roll = false;

  window.onKey = [&](int key, int scanCode, int action, int mods) -> void {
    if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)
      glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (action == GLFW_PRESS || action == GLFW_RELEASE) {
      int mag = action == GLFW_PRESS ? 1 : -1;
      if (key == GLFW_KEY_SPACE)
        velocity.y -= mag;
      else if (key == GLFW_KEY_LEFT_SHIFT)
        speed *= mag > 0 ? 5.0f : 0.2f;
      else if (key == GLFW_KEY_W)
        velocity.z += mag;
      else if (key == GLFW_KEY_S)
        velocity.z -= mag;
      else if (key == GLFW_KEY_D)
        velocity.x -= mag;
      else if (key == GLFW_KEY_A)
        velocity.x += mag;
      else if (key == GLFW_KEY_E)
        roll = mag > 0;
    }
  };

  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glm::vec<2, double> cursorPos;
  window.onCursorPos = [&](double xpos, double ypos) -> void {
    cursorPos = {xpos, ypos};
  };

  std::optional<glm::vec<2, double>> prevCursorPos;
  glm::quat rotQuat{1.0f, 0.0f, 0.0f, 0.0f};
  float mouseSensitivity = 1e-2f;

  using time_point_t = decltype(std::chrono::high_resolution_clock::now());
  auto startTime = std::chrono::high_resolution_clock::now();
  std::optional<time_point_t> prevTime;

  float fov = 45.0f, fovSensitivity = 1.0f;
  window.onScroll = [&](double xoffset, double yoffset) -> void {
    fov -= yoffset * fovSensitivity;
    fov = std::clamp(fov, 15.0f, 120.0f);
  };

  int curFrameIndex = 0;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    auto &frame = frames[curFrameIndex];

    if (prevCursorPos.has_value()) {
      auto diff = cursorPos - prevCursorPos.value();
      if (diff.x != 0 || diff.y != 0) {
        auto up = rotQuat * glm::vec3{0.0f, 1.0f, 0.0f};
        auto right = rotQuat * glm::vec3{1.0f, 0.0f, 0.0f};
        auto lookDir = rotQuat * glm::vec3{0.0f, 0.0f, 1.0f};

        auto xdir = roll ? lookDir : up;
        rotQuat = glm::angleAxis(-(float)diff.y * mouseSensitivity, right) *
                  glm::angleAxis(-(float)diff.x * mouseSensitivity, xdir) *
                  rotQuat;
        rotQuat = glm::normalize(rotQuat);
      }
    }

    auto curTime = std::chrono::high_resolution_clock::now();
    if (prevTime.has_value()) {
      using namespace std::chrono;
      auto timeDiff = curTime - prevTime.value();
      auto elapsed = duration_cast<nanoseconds>(timeDiff).count() * 1.0e-9f;

      cameraPos += rotQuat * (glm::vec3(velocity) * speed * elapsed);
    }

    prevTime = curTime;
    prevCursorPos = cursorPos;

    auto model = glm::identity<glm::mat4>();

    auto up = rotQuat * glm::vec3{0.0f, 1.0f, 0.0f},
         lookDir = rotQuat * glm::vec3{0.0f, 0.0f, 1.0f};
    auto view = glm::lookAt(cameraPos, cameraPos + lookDir, up);

    auto proj = glm::perspective(glm::radians(fov),
                                 swapExtent.width / (float)swapExtent.height,
                                 0.01f, 1000.0f);

    vsUnif.mvp = proj * view * model;
    std::memcpy(frame.unifVsPtr.get(), &vsUnif, sizeof(VSUnif));

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

      auto cmdBufRenderPass = CommandBufferRenderPass(
          cmdBufRecording,
          RenderPassBeginInfo{
              .renderPass = renderPass,
              .framebuffer = framebuffer,
              .renderArea = VkRect2D{.offset = {0, 0}, .extent = swapExtent},
              .clearValues =
                  {VkClearValue{.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
                   VkClearValue{.depthStencil = {1.0f, (uint32_t)0}}},
              .subpassContents = VK_SUBPASS_CONTENTS_INLINE});

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

      cmdBufRecording->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                          *pipelineLayout, 0,
                                          {frame.perFrameSet});

      for (uint32_t index = 0; index < submeshes.size(); ++index) {
        auto &submesh = submeshes[index];

        cmdBufRenderPass.bindPipeline(pipeline);

        auto materialSet = frame.perMaterialSets[submesh.mat];
        cmdBufRecording->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            *pipelineLayout, 1, {materialSet});

        cmdBufRenderPass.drawIndexed(submesh.count, 1, submesh.offset, 0, 0);
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