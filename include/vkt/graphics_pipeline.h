#pragma once
#include <vkt/device.h>
#include <vkt/pipeline.h>
#include <vkt/pipeline_layout.h>
#include <vkt/render_pass.h>
#include <vkt/shader_module.h>
#include <variant>
#include <array>

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

struct DepthStencilStateCreateInfo {
  VkBool32 depthTestEnable;
  VkBool32 depthWriteEnable;
  VkCompareOp depthCompareOp;
  VkBool32 depthBoundsTestEnable;
  VkBool32 stencilTestEnable;
  VkStencilOpState front;
  VkStencilOpState back;
  float minDepthBounds;
  float maxDepthBounds;
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
  DepthStencilStateCreateInfo depthStencilState;
  ColorBlendStateCreateInfo colorBlendState;
  std::vector<VkDynamicState> dynamicStates;
  std::shared_ptr<PipelineLayout> pipelineLayout;
  std::shared_ptr<RenderPass> renderPass;
  uint32_t subpass;
};

class GraphicsPipeline : public Pipeline {
public:
  GraphicsPipeline() = default;
  GraphicsPipeline(std::shared_ptr<Device> device,
                   GraphicsPipelineCreateInfo const &createInfo);

  GraphicsPipeline(GraphicsPipeline const &) = delete;
  GraphicsPipeline &operator=(GraphicsPipeline const &) = delete;

  GraphicsPipeline(GraphicsPipeline &&other) = default;
  GraphicsPipeline &operator=(GraphicsPipeline &&other) = default;

private:
  std::shared_ptr<PipelineLayout> pipelineLayout = {};
  std::shared_ptr<RenderPass> renderPass = {};
};