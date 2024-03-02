#pragma once
#include <vkt/device.h>
#include <vkt/queue.h>
#include <vkx/model.h>
#include <map>
#include <vkt/buffer.h>
#include <vkt/sampler.h>
#include <vkt/image.h>
#include <vkt/image_view.h>
#include <vkx/align.h>

struct VkMesh {
  Buffer pos, normal, color, texCoord0;
  Buffer index;
};

struct VkTexture {
  Image *image;
  ImageView imageView;
  Sampler sampler;
  VkImageLayout imageLayout;
};

#define BLEND_OP_MUL 0
#define BLEND_OP_ADD 1

struct VkTexStack {
  glm::vec3 color;
  int texIndex;
  int blendOp;
  float blendFactor;
};

struct VkMaterialData {
  VkTexStack ambient;
  VkTexStack diffuse;
  VkTexStack specular;
  float shininess;
};

struct VkMaterial {
  VkTexture ambientTex, diffuseTex, specularTex;
  VkMaterialData data;
};

class VkModel {
public:
  VkModel() = default;

  VkModel(std::shared_ptr<Device> device, ::Model const &model,
          Queue &graphicsQueue, Queue &transferQueue);

public:
  std::shared_ptr<Device> device;
  std::vector<VkMesh> meshes;
  std::map<std::string, Image> texImages;
  std::vector<VkTexture> textures;
  std::vector<VkMaterial> materials;
};