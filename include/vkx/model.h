#pragma once
#include <glm/glm.hpp>

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
  glm::vec2 texCoord;
};

struct MaterialData {
  alignas(16) glm::vec3 ambient, diffuse, specular, transmittance, emission;
  alignas(16) float shininess, ior, dissolve;
  alignas(16) int illum;
};

class Model {
public:
};