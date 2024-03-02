#pragma once
#include <cstdlib>
#include <vkx/model.h>
#include <vkt/sampler.h>
#include <vkt/pipeline_layout.h>
#include <vkt/graphics_pipeline.h>
#include <vkt/buffer.h>
#include <vkt/image.h>
#include <vkt/sampler.h>
#include <vkt/descriptor_set_layout.h>
#include <vkt/descriptor_pool.h>
#include <vkt/command_buffer.h>
#include <map>
#include <vkx/stb_image.h>
#include <vkx/missing_tex.h>
#include <vkx/align.h>
#include <vkx/auto_desc_pool.h>
#include <vkx/vk_model.h>

namespace phong {

struct VP {
  VK_ALIGN(glm::mat4) view;
  VK_ALIGN(glm::mat4) proj;
};

struct Scene {
  VK_ALIGN(glm::vec3) ambientLight;
  VK_ALIGN(glm::vec3) lightPos;
  VK_ALIGN(glm::vec3) lightColor;
  VK_ALIGN(glm::vec3) cameraPos;
  VK_ALIGN(int) shadingModel;
};

struct Model {
  VK_ALIGN(glm::mat4) model;
  VK_ALIGN(glm::mat4) normalMtx;
};

struct VkPerRender {
  Buffer vpBuf, sceneBuf;
  std::shared_ptr<VP> vp;
  std::shared_ptr<Scene> scene;
  std::vector<VkDescriptorSet> descriptorSets;
};

struct VkPerObject {
  Buffer modelBuf;
  std::shared_ptr<Model> model;
  std::vector<VkDescriptorSet> descriptorSets;
};

struct PerMesh {
  Material material;
  std::shared_ptr<Image> ambient, diffuse, specular;
};

struct VkPerMesh {
  Sampler ambient, diffuse, specular;
};

enum DescriptorSetType : uint32_t {
  PER_RENDER,
  PER_OBJECT,
  PER_MATERIAL,
  NUM_DESCRIPTOR_SET_TYPES
};

class Shader {
public:
  Shader(std::shared_ptr<Device> device, std::shared_ptr<Queue> graphicsQueue,
         std::string const &vsSource, std::string const &fsSource) {
    this->device = device;
    this->graphicsQueue = graphicsQueue;

    vertShader = std::make_shared<ShaderModule>(
        device, ShaderModuleCreateInfo{.code = vsSource});

    fragShader = std::make_shared<ShaderModule>(
        device, ShaderModuleCreateInfo{.code = fsSource});

    shaderStages = {{.stage = VK_SHADER_STAGE_VERTEX_BIT,
                     .module = vertShader,
                     .name = "main"},
                    {.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                     .module = fragShader,
                     .name = "main"}};

    vertexInputState = {.bindings =
                            {// VkMesh::pos
                             {.binding = 0,
                              .stride = sizeof(glm::vec3),
                              .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
                             // VkMesh::normal
                             {.binding = 1,
                              .stride = sizeof(glm::vec3),
                              .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
                             // VkMesh::color
                             {.binding = 2,
                              .stride = sizeof(glm::vec3),
                              .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
                             // VkMesh::texCoord0
                             {.binding = 2,
                              .stride = sizeof(glm::vec2),
                              .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}},
                        .attributes = {// in vec3 inPosition;
                                       {.location = 0,
                                        .binding = 0,
                                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                                        .offset = 0},
                                       // in vec3 inColor;
                                       {.location = 1,
                                        .binding = 1,
                                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                                        .offset = 0},
                                       // in vec3 inNormal;
                                       {.location = 2,
                                        .binding = 2,
                                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                                        .offset = 0},
                                       // in vec3 inTexCoord;
                                       {.location = 3,
                                        .binding = 3,
                                        .format = VK_FORMAT_R32G32_SFLOAT,
                                        .offset = 0}}};

    std::vector<VkDescriptorSetLayoutBinding>
        bindings[NUM_DESCRIPTOR_SET_TYPES];

    bindings[PER_RENDER] = {
        // vp: VP
        {.binding = 0,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_VERTEX_BIT},
        // scene: Scene
        {.binding = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
    };

    bindings[PER_OBJECT] = {
        // model: Model
        {.binding = 0,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_VERTEX_BIT}};

    bindings[PER_MATERIAL] = {
        // material: Material
        {.binding = 0,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .descriptorCount = 0,
         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
        // ambient: sampler2D
        {.binding = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .descriptorCount = 0,
         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
        // diffuse: sampler2D
        {.binding = 2,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .descriptorCount = 0,
         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
        // specular: sampler2D
        {.binding = 3,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .descriptorCount = 0,
         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
    };

    for (uint32_t type = 0; type < NUM_DESCRIPTOR_SET_TYPES; ++type) {
      setLayouts.emplace_back(
          device, DescriptorSetLayoutCreateInfo{.bindings = bindings[type]});
    }

    std::vector<VkDescriptorSetLayout> vkSetLayouts;
    for (auto &setLayout : setLayouts)
      vkSetLayouts.push_back(setLayout);

    pipelineLayout = std::make_shared<PipelineLayout>(
        device, PipelineLayoutCreateInfo{.setLayouts = vkSetLayouts,
                                         .pushConstantRanges = {}});
  }

  VkPerRender perRenderSet() {
    VkPerRender perRender;

    perRender.vpBuf =
        Buffer(device,
               BufferCreateInfo{
                   .size = sizeof(VP),
                   .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                   .queueFamilyIndices = {graphicsQueue->getQueueFamilyIndex()},
               });
    auto &vpMem =
        perRender.vpBuf.allocMemory(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    perRender.vp = std::static_pointer_cast<VP, void>(vpMem.map());

    perRender.sceneBuf =
        Buffer(device,
               BufferCreateInfo{
                   .size = sizeof(Scene),
                   .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                   .queueFamilyIndices = {graphicsQueue->getQueueFamilyIndex()},
               });
    auto &sceneMem =
        perRender.sceneBuf.allocMemory(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    perRender.scene = std::static_pointer_cast<Scene, void>(sceneMem.map());

    auto set = autoPool.create(PER_RENDER);

    device->updateDescriptorSets(
        {WriteDescriptorSet{
             .dstSet = set,
             .dstBinding = 0,
             .dstArrayElement = 0,
             .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
             .bufferInfos = {VkDescriptorBufferInfo{
                 .buffer = perRender.vpBuf, .offset = 0, .range = sizeof(VP)}}},
         WriteDescriptorSet{.dstSet = set,
                            .dstBinding = 1,
                            .dstArrayElement = 0,
                            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            .bufferInfos = {VkDescriptorBufferInfo{
                                .buffer = perRender.sceneBuf,
                                .offset = 0,
                                .range = sizeof(Scene)}}}});

    return perRender;
  }

  VkPerObject perObjectSet() {
    VkPerObject perObject;

    perObject.modelBuf =
        Buffer(device,
               BufferCreateInfo{
                   .size = sizeof(Model),
                   .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                   .queueFamilyIndices = {graphicsQueue->getQueueFamilyIndex()},
               });
    auto &modelMem =
        perObject.modelBuf.allocMemory(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    perObject.model = std::static_pointer_cast<Model, void>(modelMem.map());

    auto set = autoPool.create(PER_OBJECT);

    device->updateDescriptorSets({WriteDescriptorSet{
        .dstSet = set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .bufferInfos = {VkDescriptorBufferInfo{.buffer = perObject.modelBuf,
                                               .offset = 0,
                                               .range = sizeof(Model)}}}});

    return perObject;
  }

  VkDescriptorSet perMaterialSet(VkMaterial &vkMat) {
    auto set = autoPool.create(PER_MATERIAL);

    device->updateDescriptorSets(
        {WriteDescriptorSet{.dstSet = set,
                            .dstBinding = 0,
                            .dstArrayElement = 0,
                            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            .bufferInfos = {VkDescriptorBufferInfo{
                                .buffer = vkMat.materialBuf,
                                .offset = 0,
                                .range = sizeof(Material)}}},
         WriteDescriptorSet{.dstSet = set,
                            .dstBinding = 1,
                            .dstArrayElement = 0,
                            .descriptorType =
                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            .imageInfos = {VkDescriptorImageInfo{
                                .sampler = vkMat.ambientTex.sampler,
                                .imageView = vkMat.ambientTex.imageView,
                                .imageLayout = vkMat.ambientTex.imageLayout}}},
         WriteDescriptorSet{.dstSet = set,
                            .dstBinding = 2,
                            .dstArrayElement = 0,
                            .descriptorType =
                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            .imageInfos = {VkDescriptorImageInfo{
                                .sampler = vkMat.diffuseTex.sampler,
                                .imageView = vkMat.diffuseTex.imageView,
                                .imageLayout = vkMat.diffuseTex.imageLayout}}},
         WriteDescriptorSet{
             .dstSet = set,
             .dstBinding = 3,
             .dstArrayElement = 0,
             .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
             .imageInfos = {VkDescriptorImageInfo{
                 .sampler = vkMat.specularTex.sampler,
                 .imageView = vkMat.specularTex.imageView,
                 .imageLayout = vkMat.specularTex.imageLayout}}}});

    return set;
  }

public:
  std::shared_ptr<Device> device;
  std::shared_ptr<Queue> graphicsQueue;
  std::shared_ptr<ShaderModule> vertShader, fragShader;
  VertexInputStateCreateInfo vertexInputState;
  AutoDescriptorPool autoPool;
  std::vector<DescriptorSetLayout> setLayouts;
  std::shared_ptr<PipelineLayout> pipelineLayout;
  std::vector<ShaderStageCreateInfo> shaderStages;
};

} // namespace phong