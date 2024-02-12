#include <vkt/graphics_pipeline.h>

GraphicsPipeline::GraphicsPipeline(
    std::shared_ptr<Device> device,
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
              .mapEntryCount = (uint32_t)specializationInfo->mapEntries.size(),
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

  auto const &depthStencilState = createInfo.depthStencilState;
  auto vk_depthStencilState = VkPipelineDepthStencilStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = {},
      .depthTestEnable = depthStencilState.depthTestEnable,
      .depthWriteEnable = depthStencilState.depthWriteEnable,
      .depthCompareOp = depthStencilState.depthCompareOp,
      .depthBoundsTestEnable = depthStencilState.depthBoundsTestEnable,
      .stencilTestEnable = depthStencilState.stencilTestEnable,
      .front = depthStencilState.front,
      .back = depthStencilState.back,
      .minDepthBounds = depthStencilState.minDepthBounds,
      .maxDepthBounds = depthStencilState.maxDepthBounds};

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
      .pDepthStencilState = &vk_depthStencilState,
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