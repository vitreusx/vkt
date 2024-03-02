#include <vkx/vk_model.h>
#include <vkx/stb_image.h>
#include <vkx/missing_tex.h>

VkModel::VkModel(std::shared_ptr<Device> device, ::Model const &model,
                 Queue &graphicsQueue, Queue &transferQueue) {
  this->device = device;
  auto graphicsQueueIndex = graphicsQueue.getQueueFamilyIndex(),
       transferQueueIndex = transferQueue.getQueueFamilyIndex();

  for (auto const &mesh : model.meshes) {
    auto &vkMesh = meshes.emplace_back();

    std::vector<std::tuple<void *, size_t, Buffer *>> buffers = {
        {(void *)mesh.pos.data(), mesh.pos.size() * sizeof(mesh.pos[0]),
         &vkMesh.pos},
        {(void *)mesh.color.data(), mesh.color.size() * sizeof(mesh.color[0]),
         &vkMesh.color},
        {(void *)mesh.normal.data(), mesh.normal.size() * sizeof(glm::vec3),
         &vkMesh.normal},
        {(void *)mesh.texCoord[0].data(),
         mesh.texCoord[0].size() * sizeof(glm::vec2), &vkMesh.texCoord0},
        {(void *)mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t),
         &vkMesh.index}};

    for (auto const &[data, size, bufPtr] : buffers) {
      *bufPtr = Buffer(
          device, BufferCreateInfo{.size = size,
                                   .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                   .sharingMode = VK_SHARING_MODE_CONCURRENT,
                                   .queueFamilyIndices = {graphicsQueueIndex,
                                                          transferQueueIndex}});
      bufPtr->allocMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      bufPtr->stage(data, size, transferQueue);
    }
  }

  auto getTexImage = [&](std::string const &path) -> Image & {
    auto iter = texImages.find(path);
    if (iter == texImages.end()) {
      StbImage *texImage;
      std::unique_ptr<StbImage> texImageResource;
      if (path.empty()) {
        texImage = &missingTexImage();
      } else {
        texImageResource = std::make_unique<StbImage>(path, STBI_rgb_alpha);
        texImage = texImageResource.get();
      }

      auto texFormat =
          device->physDev
              .findSuitableFormat({VK_FORMAT_R8G8B8A8_SRGB},
                                  VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
              .value();

      auto texture = Image(
          device,
          ImageCreateInfo{
              .imageType = VK_IMAGE_TYPE_2D,
              .format = texFormat,
              .extent = VkExtent3D{.width = (uint32_t)texImage->width,
                                   .height = (uint32_t)texImage->height,
                                   .depth = 1},
              .mipLevels = 1,
              .arrayLayers = 1,
              .samples = VK_SAMPLE_COUNT_1_BIT,
              .tiling = VK_IMAGE_TILING_OPTIMAL,
              .usage =
                  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
              .sharingMode = VK_SHARING_MODE_CONCURRENT,
              .queueFamilyIndices = {graphicsQueueIndex, transferQueueIndex},
              .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED});

      texture.allocMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      texture.stage(texImage->data, texImage->nbytes(), transferQueue,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

      texImages[path] = std::move(texture);
      iter = texImages.find(path);
    }
    return iter->second;
  };

  for (auto const &mat : model.materials) {
    auto &vkMat = materials.emplace_back();

    VkMaterialData matData;
    matData.shininess = mat.shininess;

    std::vector<std::tuple<VkTexStack *, VkTexture *, TextureStack const *>>
        texTypes = {{&matData.ambient, &vkMat.ambientTex, &mat.ambient},
                    {&matData.diffuse, &vkMat.diffuseTex, &mat.diffuse},
                    {&matData.specular, &vkMat.specularTex, &mat.specular}};

    for (auto &[vkTexStack, vkTex, texStack] : texTypes) {
      vkTexStack->color = texStack->baseColor;

      Image *texImage = &getTexImage("");
      vkTexStack->texIndex = -1;
      if (!texStack->layers.empty()) {
        auto const &layer = texStack->layers[0];

        vkTexStack->blendFactor = layer.blend;
        switch (layer.op) {
        case aiTextureOp_Multiply:
          vkTexStack->blendOp = BLEND_OP_MUL;
          break;
        case aiTextureOp_Add:
          vkTexStack->blendOp = BLEND_OP_ADD;
          break;
        default:
          throw std::runtime_error("Unknown blend op");
        }

        texImage = &getTexImage(layer.texPath.string());
      }

      vkTex->image = texImage;
      vkTex->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      vkTex->imageView = ImageView(
          device,
          ImageViewCreateInfo{
              .image = *vkTex->image,
              .viewType = VK_IMAGE_VIEW_TYPE_2D,
              .format = vkTex->image->createInfo.format,
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

      vkTex->sampler = Sampler(
          device,
          VkSamplerCreateInfo{
              .magFilter = VK_FILTER_LINEAR,
              .minFilter = VK_FILTER_LINEAR,
              .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
              .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
              .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
              .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
              .mipLodBias = 0.0f,
              .anisotropyEnable = VK_TRUE,
              .maxAnisotropy =
                  device->physDev.properties.limits.maxSamplerAnisotropy,
              .compareEnable = VK_FALSE,
              .compareOp = VK_COMPARE_OP_ALWAYS,
              .minLod = 0.0f,
              .maxLod = 0.0f,
              .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
              .unnormalizedCoordinates = VK_FALSE});
    }

    vkMat.data = matData;
  }
}