#pragma once
#include <filesystem>
#include <tiny_obj_loader.h>

class ObjModel {
public:
  ObjModel(std::filesystem::path const &path);

  std::filesystem::path path, root;
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
};