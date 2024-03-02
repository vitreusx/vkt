#pragma once
#include <glm/glm.hpp>
#include <filesystem>
#include <assimp/material.h>
#include <assimp/texture.h>

struct Mesh {
  std::vector<glm::vec3> pos, normal, color;
  std::vector<std::vector<glm::vec2>> texCoord;
  std::vector<uint32_t> indices;
};

struct TextureStackLayer {
  aiTextureOp op;
  ai_real blend;
  std::filesystem::path texPath;
  int uvChannel;
  aiTextureMapMode mapModeU, mapModeV;
};

struct TextureStack {
  glm::vec4 baseColor;
  std::vector<TextureStackLayer> layers;
};

struct Material {
  TextureStack ambient, diffuse, specular, transmission, emission;
  ai_real shininess, ior, opacity;
  aiShadingMode shadingModel;
};

class Model {
public:
  Model(std::filesystem::path const &path);
  std::vector<Mesh> meshes;
  std::vector<Material> materials;
};